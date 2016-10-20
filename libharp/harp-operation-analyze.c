/*
 * Copyright (C) 2015-2016 S[&]T, The Netherlands.
 *
 * This file is part of HARP.
 *
 * HARP is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HARP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HARP; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "harp-internal.h"
#include "harp-operation.h"
#include "harp-operation-parse.h"
#include "harp-program.h"
#include "harp-parser.h"
#include "harp-parser-state.h"
#include "harp-scanner.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef int create_func(const ast_node *argument_list, harp_operation **new_operation);

#define MAX_NUM_FUNCTION_ARGUMENTS 5

typedef struct function_prototype_struct
{
    const char *name;
    int num_arguments;
    ast_node_type argument_type[MAX_NUM_FUNCTION_ARGUMENTS];
    create_func *create_func;
} function_prototype;

static int create_collocation_filter_left(const ast_node *argument_list, harp_operation **new_operation);
static int create_collocation_filter_right(const ast_node *argument_list, harp_operation **new_operation);
static int create_valid_range_filter(const ast_node *argument_list, harp_operation **new_operation);
static int create_longitude_range_filter(const ast_node *argument_list, harp_operation **new_operation);
static int create_point_distance_filter(const ast_node *argument_list, harp_operation **new_operation);
static int create_area_mask_covers_point_filter(const ast_node *argument_list, harp_operation **new_operation);
static int create_area_mask_covers_area_filter(const ast_node *argument_list, harp_operation **new_operation);
static int create_area_mask_intersects_area_filter(const ast_node *argument_list, harp_operation **new_operation);
static int create_variable_derivation(const ast_node *argument_list, harp_operation **new_operation);
static int create_variable_inclusion(const ast_node *argument_list, harp_operation **new_operation);
static int create_variable_exclusion(const ast_node *argument_list, harp_operation **new_operation);
static int create_regrid(const ast_node *argument_list, harp_operation **new_operation);
static int create_regrid_collocated(const ast_node *argument_list, harp_operation **new_operation);
static int create_flatten(const ast_node *argument_list, harp_operation **new_operation);

#define NUM_BUILTIN_FUNCTIONS 14
static function_prototype builtin_function[NUM_BUILTIN_FUNCTIONS] = {
    {"collocate-left", 1, {ast_string}, &create_collocation_filter_left},
    {"collocate-right", 1, {ast_string}, &create_collocation_filter_right},
    {"valid", 1, {ast_qualified_name}, &create_valid_range_filter},
    {"longitude-range", 2, {ast_quantity, ast_quantity}, &create_longitude_range_filter},
    {"point-distance", 3, {ast_quantity, ast_quantity, ast_quantity}, &create_point_distance_filter},
    {"area-mask-covers-point", 1, {ast_string}, &create_area_mask_covers_point_filter},
    {"area-mask-covers-area", 1, {ast_string}, &create_area_mask_covers_area_filter},
    {"area-mask-intersects-area", 2, {ast_string, ast_quantity}, &create_area_mask_intersects_area_filter},
    {"derive", 1, {ast_qualified_name}, &create_variable_derivation},
    {"keep", -1, {0}, &create_variable_inclusion},
    {"exclude", -1, {0}, &create_variable_exclusion},
    {"regrid", 1, {ast_string}, &create_regrid},
    {"regrid_collocated", 4, {ast_string, ast_string, ast_qualified_name, ast_qualified_name},
     &create_regrid_collocated},
    {"flatten", 1, {ast_qualified_name}, &create_flatten},
};

static function_prototype *get_function_prototype_by_name(const char *name)
{
    int i;

    for (i = 0; i < NUM_BUILTIN_FUNCTIONS; i++)
    {
        if (strcmp(builtin_function[i].name, name) == 0)
        {
            return &builtin_function[i];
        }
    }

    return NULL;
}

static int is_homogeneous_list(const ast_node *node)
{
    ast_node_type node_type;
    int i;

    if (node->type != ast_list)
    {
        return 0;
    }

    if (node->num_child_nodes == 0)
    {
        return 1;
    }

    node_type = node->child_node[0]->type;
    for (i = 1; i < node->num_child_nodes; i++)
    {
        if (node->child_node[i]->type != node_type)
        {
            return 0;
        }
    }

    return 1;
}

static harp_comparison_operator_type get_operator_type(ast_node_type node_type)
{
    switch (node_type)
    {
        case ast_eq:
            return harp_operator_eq;
        case ast_ne:
            return harp_operator_ne;
        case ast_lt:
            return harp_operator_lt;
        case ast_le:
            return harp_operator_le;
        case ast_gt:
            return harp_operator_gt;
        case ast_ge:
            return harp_operator_ge;
        default:
            assert(0);
            exit(1);
    }
}

static const char *get_unit(const ast_node *unit)
{
    assert(unit == NULL || unit->type == ast_unit);
    return unit == NULL ? NULL : unit->payload.string;
}

static int get_dimension_list(const ast_node *dimension_list, int *num_dimensions, harp_dimension_type *dimension_type)
{
    int i;

    assert(dimension_list->type == ast_dimension_list);

    if (dimension_list->num_child_nodes > HARP_MAX_NUM_DIMS)
    {
        harp_set_error(HARP_ERROR_OPERATION, "char %lu: maximum number of dimensions exceeded",
                       dimension_list->child_node[HARP_MAX_NUM_DIMS]->position);
        return -1;
    }
    *num_dimensions = dimension_list->num_child_nodes;

    for (i = 0; i < dimension_list->num_child_nodes; i++)
    {
        const ast_node *child_node;

        child_node = dimension_list->child_node[i];
        if (harp_parse_dimension_type(child_node->payload.string, &dimension_type[i]) != 0)
        {
            harp_set_error(HARP_ERROR_OPERATION, "char %lu: unknown dimension type '%s'", child_node->position,
                           child_node->payload.string);
            return -1;
        }
    }

    return 0;
}

static int verify_qualified_name_has_no_qualifiers(const ast_node *qualified_name)
{
    const ast_node *dimension_list;
    const ast_node *unit;

    assert(qualified_name->type == ast_qualified_name);
    assert(qualified_name->num_child_nodes == 3);

    dimension_list = qualified_name->child_node[1];
    unit = qualified_name->child_node[2];

    if (dimension_list != NULL)
    {
        harp_set_error(HARP_ERROR_OPERATION, "char %lu: unexpected dimension list", dimension_list->position);
        return -1;
    }

    if (unit != NULL)
    {
        harp_set_error(HARP_ERROR_OPERATION, "char %lu: unexpected unit", unit->position);
        return -1;
    }

    return 0;
}

static int verify_quantity_has_no_unit(const ast_node *quantity)
{
    const ast_node *unit;

    assert(quantity->type == ast_quantity);
    assert(quantity->num_child_nodes == 2);

    unit = quantity->child_node[1];

    if (unit != NULL)
    {
        harp_set_error(HARP_ERROR_OPERATION, "char %lu: unexpected unit", unit->position);
        return -1;
    }

    return 0;
}

static void split_quantity(const ast_node *quantity, double *value, const char **unit)
{
    assert(quantity->type == ast_quantity);
    assert(quantity->num_child_nodes == 2);

    if (value != NULL)
    {
        *value = quantity->child_node[0]->payload.number;
    }

    if (unit != NULL)
    {
        *unit = get_unit(quantity->child_node[1]);
    }
}

static int create_collocation_filter_left(const ast_node *argument_list, harp_operation **new_operation)
{
    const ast_node *name;

    name = argument_list->child_node[0];
    return harp_collocation_filter_new(name->payload.string, harp_collocation_left, new_operation);
}

static int create_collocation_filter_right(const ast_node *argument_list, harp_operation **new_operation)
{
    const ast_node *name;

    name = argument_list->child_node[0];
    return harp_collocation_filter_new(name->payload.string, harp_collocation_right, new_operation);
}

static int create_valid_range_filter(const ast_node *argument_list, harp_operation **new_operation)
{
    const ast_node *qualified_name;
    const ast_node *name;

    qualified_name = argument_list->child_node[0];
    if (verify_qualified_name_has_no_qualifiers(qualified_name) != 0)
    {
        return -1;
    }

    name = qualified_name->child_node[0];
    return harp_valid_range_filter_new(name->payload.string, new_operation);
}

static int create_longitude_range_filter(const ast_node *argument_list, harp_operation **new_operation)
{
    double longitude_min;
    const char *longitude_min_unit;
    double longitude_max;
    const char *longitude_max_unit;

    split_quantity(argument_list->child_node[0], &longitude_min, &longitude_min_unit);
    split_quantity(argument_list->child_node[1], &longitude_max, &longitude_max_unit);
    return harp_longitude_range_filter_new(longitude_min, longitude_min_unit, longitude_max, longitude_max_unit,
                                           new_operation);
}

static int create_point_distance_filter(const ast_node *argument_list, harp_operation **new_operation)
{
    double longitude;
    const char *longitude_unit;
    double latitude;
    const char *latitude_unit;
    double distance;
    const char *distance_unit;

    split_quantity(argument_list->child_node[0], &longitude, &longitude_unit);
    split_quantity(argument_list->child_node[1], &latitude, &latitude_unit);
    split_quantity(argument_list->child_node[2], &distance, &distance_unit);

    return harp_point_distance_filter_new(longitude, longitude_unit, latitude, latitude_unit, distance, distance_unit,
                                          new_operation);
}

static int create_area_mask_covers_point_filter(const ast_node *argument_list, harp_operation **new_operation)
{
    const ast_node *name;

    name = argument_list->child_node[0];
    return harp_area_mask_covers_point_filter_new(name->payload.string, new_operation);
}

static int create_area_mask_covers_area_filter(const ast_node *argument_list, harp_operation **new_operation)
{
    const ast_node *name;

    name = argument_list->child_node[0];
    return harp_area_mask_covers_area_filter_new(name->payload.string, new_operation);
}

static int create_area_mask_intersects_area_filter(const ast_node *argument_list, harp_operation **new_operation)
{
    const ast_node *name;
    const ast_node *quantity;
    double percentage;

    name = argument_list->child_node[0];
    quantity = argument_list->child_node[1];

    if (verify_quantity_has_no_unit(quantity) != 0)
    {
        return -1;
    }

    percentage = quantity->child_node[0]->payload.number;

    return harp_area_mask_intersects_area_filter_new(name->payload.string, percentage, new_operation);
}

static int create_variable_derivation(const ast_node *argument_list, harp_operation **new_operation)
{
    const ast_node *qualified_name;
    const ast_node *dimension_list;
    const char *variable_name;
    int num_dimensions;
    harp_dimension_type dimension_type[HARP_MAX_NUM_DIMS];
    const char *unit;

    qualified_name = argument_list->child_node[0];
    assert(qualified_name->num_child_nodes == 3);

    variable_name = qualified_name->child_node[0]->payload.string;

    dimension_list = qualified_name->child_node[1];
    if (dimension_list == NULL)
    {
        harp_set_error(HARP_ERROR_OPERATION, "char %lu: expected dimension list", qualified_name->position);
        return -1;
    }

    if (get_dimension_list(dimension_list, &num_dimensions, dimension_type) != 0)
    {
        return -1;
    }

    unit = get_unit(qualified_name->child_node[2]);
    return harp_variable_derivation_new(variable_name, num_dimensions, dimension_type, unit, new_operation);
}

static int create_variable_inclusion(const ast_node *argument_list, harp_operation **new_operation)
{
    const char **name_list;
    int i;

    /* Check variable argument list. */
    if (argument_list->num_child_nodes == 0)
    {
        harp_set_error(HARP_ERROR_OPERATION, "char %lu: function expects one or more arguments",
                       argument_list->position);
        return -1;
    }

    for (i = 0; i < argument_list->num_child_nodes; i++)
    {
        const ast_node *argument;

        argument = argument_list->child_node[i];
        if (argument->type != ast_qualified_name)
        {
            harp_set_error(HARP_ERROR_OPERATION, "char %lu: invalid argument type", argument->position);
            return -1;
        }

        if (verify_qualified_name_has_no_qualifiers(argument) != 0)
        {
            return -1;
        }
    }

    /* Create variable inclusion operation. */
    name_list = (const char **)malloc(argument_list->num_child_nodes * sizeof(const char *));
    if (name_list == NULL)
    {
        harp_set_error(HARP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       argument_list->num_child_nodes * sizeof(const char *), __FILE__, __LINE__);
        return -1;
    }

    for (i = 0; i < argument_list->num_child_nodes; i++)
    {
        const ast_node *name;

        name = argument_list->child_node[i]->child_node[0];
        assert(name->type == ast_name);
        name_list[i] = name->payload.string;
    }

    if (harp_variable_inclusion_new(argument_list->num_child_nodes, name_list, new_operation) != 0)
    {
        free(name_list);
        return -1;
    }

    free(name_list);
    return 0;
}

static int create_variable_exclusion(const ast_node *argument_list, harp_operation **new_operation)
{
    const char **name_list;
    int i;

    /* Check variable argument list. */
    if (argument_list->num_child_nodes == 0)
    {
        harp_set_error(HARP_ERROR_OPERATION, "char %lu: function expects one or more arguments",
                       argument_list->position);
        return -1;
    }

    for (i = 0; i < argument_list->num_child_nodes; i++)
    {
        const ast_node *argument;

        argument = argument_list->child_node[i];
        if (argument->type != ast_qualified_name)
        {
            harp_set_error(HARP_ERROR_OPERATION, "char %lu: invalid argument type", argument->position);
            return -1;
        }

        if (verify_qualified_name_has_no_qualifiers(argument) != 0)
        {
            return -1;
        }
    }

    /* Create variable exclusion operation. */
    name_list = (const char **)malloc(argument_list->num_child_nodes * sizeof(const char *));
    if (name_list == NULL)
    {
        harp_set_error(HARP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       argument_list->num_child_nodes * sizeof(const char *), __FILE__, __LINE__);
        return -1;
    }

    for (i = 0; i < argument_list->num_child_nodes; i++)
    {
        const ast_node *name;

        name = argument_list->child_node[i]->child_node[0];
        assert(name->type == ast_name);
        name_list[i] = name->payload.string;
    }

    if (harp_variable_exclusion_new(argument_list->num_child_nodes, name_list, new_operation) != 0)
    {
        free(name_list);
        return -1;
    }

    free(name_list);
    return 0;
}

static int create_comparison(ast_node *node, harp_operation **new_operation)
{
    harp_operation *operation;

    assert(node->num_child_nodes == 2);
    if (node->child_node[1]->type == ast_string)
    {
        if (node->type != ast_eq && node->type != ast_ne)
        {
            harp_set_error(HARP_ERROR_OPERATION, "char %lu: operator not supported for strings", node->position);
            return -1;
        }

        if (harp_string_comparison_filter_new(node->child_node[0]->payload.string, get_operator_type(node->type),
                                              node->child_node[1]->payload.string, &operation) != 0)
        {
            return -1;
        }
    }
    else
    {
        double value;
        const char *unit;

        if (node->child_node[1]->type == ast_number)
        {
            value = node->child_node[1]->payload.number;
            unit = NULL;
        }
        else
        {
            ast_node *quantity = node->child_node[1];

            value = quantity->child_node[0]->payload.number;
            unit = quantity->child_node[1] == NULL ? NULL : quantity->child_node[1]->payload.string;
        }

        if (harp_comparison_filter_new(node->child_node[0]->payload.string, get_operator_type(node->type), value, unit,
                                       &operation) != 0)
        {
            return -1;
        }
    }

    *new_operation = operation;
    return 0;
}

static int create_bit_mask_test(ast_node *node, harp_operation **new_operation)
{
    harp_operation *operation;
    harp_bit_mask_operator_type operator_type;
    uint32_t bit_mask;

    /* Get membership test operator type. */
    assert(node->type == ast_bit_mask_any || node->type == ast_bit_mask_none);
    if (node->type == ast_bit_mask_any)
    {
        operator_type = harp_operator_bit_mask_any;
    }
    else
    {
        operator_type = harp_operator_bit_mask_none;
    }

    assert(node->num_child_nodes == 2);
    assert(node->child_node[1]->type == ast_number);
    bit_mask = (uint32_t)node->child_node[1]->payload.number;

    if (harp_bit_mask_filter_new(node->child_node[0]->payload.string, operator_type, bit_mask, &operation) != 0)
    {
        return -1;
    }

    *new_operation = operation;
    return 0;
}

static int create_membership_test(ast_node *node, harp_operation **new_operation)
{
    harp_operation *operation;
    harp_membership_operator_type operator_type;
    const ast_node *name;
    const ast_node *list;
    const ast_node *unit;

    assert(node->num_child_nodes == 3);

    /* Get membership test operator type. */
    assert(node->type == ast_in || node->type == ast_not_in);
    if (node->type == ast_in)
    {
        operator_type = harp_operator_in;
    }
    else
    {
        operator_type = harp_operator_not_in;
    }

    /* Get name. */
    name = node->child_node[0];

    /* Get value list. */
    list = node->child_node[1];
    assert(list->type == ast_list);
    assert(list->num_child_nodes > 0);

    /* Get unit. */
    unit = node->child_node[2];

    /* A string list cannot be qualified with a unit. */
    if (list->child_node[0]->type == ast_string && unit != NULL)
    {
        harp_set_error(HARP_ERROR_OPERATION, "char %lu: unexpected unit", unit->position);
        return -1;
    }

    /* All values in the value list should be of the same type. */
    if (!is_homogeneous_list(list))
    {
        harp_set_error(HARP_ERROR_OPERATION, "char %lu: values in list should be of the same type", list->position);
        return -1;
    }

    if (list->child_node[0]->type == ast_string)
    {
        const char **string_list;
        int i;

        string_list = (const char **)malloc(list->num_child_nodes * sizeof(const char *));
        if (string_list == NULL)
        {
            harp_set_error(HARP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           list->num_child_nodes * sizeof(const char *), __FILE__, __LINE__);
            return -1;
        }

        for (i = 0; i < list->num_child_nodes; i++)
        {
            assert(list->child_node[i]->type == ast_string);
            string_list[i] = list->child_node[i]->payload.string;
        }

        if (harp_string_membership_filter_new(name->payload.string, operator_type, list->num_child_nodes, string_list,
                                              &operation) != 0)
        {
            free(string_list);
            return -1;
        }

        free(string_list);
    }
    else
    {
        double *double_list;
        int i;

        assert(list->child_node[0]->type == ast_number);

        double_list = (double *)malloc(list->num_child_nodes * sizeof(double));
        if (double_list == NULL)
        {
            harp_set_error(HARP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           list->num_child_nodes * sizeof(double), __FILE__, __LINE__);
            return -1;
        }

        for (i = 0; i < list->num_child_nodes; i++)
        {
            assert(list->child_node[i]->type == ast_number);
            double_list[i] = list->child_node[i]->payload.number;
        }

        if (harp_membership_filter_new(name->payload.string, operator_type, list->num_child_nodes, double_list,
                                       get_unit(unit), &operation) != 0)
        {
            free(double_list);
            return -1;
        }

        free(double_list);
    }

    *new_operation = operation;
    return 0;
}

static int create_regrid(const ast_node *argument_list, harp_operation **new_operation)
{
    const ast_node *name;

    name = argument_list->child_node[0];

    return harp_regrid_new(name->payload.string, new_operation);
}

static int create_regrid_collocated(const ast_node *argument_list, harp_operation **new_operation)
{
    const ast_node *collocation_result = argument_list->child_node[0];
    const ast_node *dataset_dir = argument_list->child_node[1];
    const ast_node *target_dataset = argument_list->child_node[2];
    const ast_node *vertical_axis = argument_list->child_node[3];

    verify_qualified_name_has_no_qualifiers(target_dataset);
    verify_qualified_name_has_no_qualifiers(vertical_axis);

    target_dataset = target_dataset->child_node[0];
    vertical_axis = vertical_axis->child_node[0];

    if (strlen(target_dataset->payload.string) > 1)
    {
        harp_set_error(HARP_ERROR_OPERATION, "char %lu: expected 'a' or 'b' for target_dataset argument, got %s",
                       target_dataset->payload.string, target_dataset->position);
        return -1;
    }

    return harp_regrid_collocated_new(collocation_result->payload.string, dataset_dir->payload.string,
                                      target_dataset->payload.string[0], vertical_axis->payload.string, new_operation);
}

static int create_flatten(const ast_node *argument_list, harp_operation **new_operation)
{
    const ast_node *argument;
    harp_dimension_type dim_type;

    argument = argument_list->child_node[0];
    if (argument->type != ast_qualified_name)
    {
        harp_set_error(HARP_ERROR_OPERATION, "char %lu: invalid argument type", argument->position);
        return -1;
    }

    if (verify_qualified_name_has_no_qualifiers(argument) != 0)
    {
        return -1;
    }

    if (harp_parse_dimension_type(argument->child_node[0]->payload.string, &dim_type) != 0)
    {
        return -1;
    }

    return harp_flatten_new(dim_type, new_operation);
}

static int operation_from_function_call(ast_node *node, harp_operation **new_operation)
{
    ast_node *function_name;
    ast_node *argument_list;
    function_prototype *prototype;
    int i;

    assert(node->type == ast_function_call);
    assert(node->num_child_nodes == 2);

    function_name = node->child_node[0];
    argument_list = node->child_node[1];

    prototype = get_function_prototype_by_name(function_name->payload.string);
    if (prototype == NULL)
    {
        harp_set_error(HARP_ERROR_OPERATION, "char %lu: undefined function '%s'", function_name->position,
                       function_name->payload.string);
        return -1;
    }

    if (prototype->num_arguments >= 0)
    {
        if (argument_list->num_child_nodes != prototype->num_arguments)
        {
            harp_set_error(HARP_ERROR_OPERATION, "char %lu: function expects %d argument(s)", argument_list->position,
                           prototype->num_arguments);
            return -1;
        }

        for (i = 0; i < prototype->num_arguments; i++)
        {
            ast_node *argument;

            argument = argument_list->child_node[i];
            if (argument->type != prototype->argument_type[i])
            {
                harp_set_error(HARP_ERROR_OPERATION, "char %lu: invalid argument type", argument->position);
                return -1;
            }
        }
    }

    return prototype->create_func(argument_list, new_operation);
}

static int create_program(ast_node *node, harp_program **new_program)
{
    harp_program *program;
    int i;

    if (harp_program_new(&program) != 0)
    {
        return -1;
    }

    for (i = 0; i < node->num_child_nodes; i++)
    {
        harp_operation *operation;

        if (node->child_node[i]->type == ast_function_call)
        {
            if (operation_from_function_call(node->child_node[i], &operation) != 0)
            {
                harp_program_delete(program);
                return -1;
            }
        }
        else if (node->child_node[i]->type == ast_in || node->child_node[i]->type == ast_not_in)
        {
            if (create_membership_test(node->child_node[i], &operation) != 0)
            {
                harp_program_delete(program);
                return -1;
            }
        }
        else if (node->child_node[i]->type == ast_bit_mask_any || node->child_node[i]->type == ast_bit_mask_none)
        {
            if (create_bit_mask_test(node->child_node[i], &operation) != 0)
            {
                harp_program_delete(program);
                return -1;
            }
        }
        else
        {
            if (create_comparison(node->child_node[i], &operation) != 0)
            {
                harp_program_delete(program);
                return -1;
            }
        }

        if (harp_program_add_operation(program, operation) != 0)
        {
            harp_operation_delete(operation);
            harp_program_delete(program);
            return -1;
        }
    }

    *new_program = program;
    return 0;
}

int harp_program_from_string(const char *str, harp_program **new_program)
{
    harp_program *program;
    harp_parser_state *state;
    YY_BUFFER_STATE buf;
    yyscan_t scanner;
    void* operationParser;
    int lexCode;
    char *last_token, *text = NULL;
    int i;

    // set up the parser state
    if (harp_parser_state_new(&state))
    {
        return -1;
    }

    // Set up the scanner
    yylex_init(&scanner);
    // yyset_in(stdin, scanner);
    buf = yy_scan_string(str, scanner);

    // Set up the parser
    operationParser = ParseAlloc(malloc);

    // Do it!
    do {
        last_token = text;
        lexCode = yylex(scanner);
        /* TODO cleanup memory leak */
        text = strdup(yyget_text(scanner));
        Parse(operationParser, lexCode, text, state);
    } while (lexCode > 0 && !state->hasError);

    if (-1 == lexCode) {
        fprintf(stderr, "The scanner encountered an error after '%s'\n", last_token);

        // Cleanup the scanner and parser
        yy_delete_buffer(buf, scanner);
        yylex_destroy(scanner);
        ParseFree(operationParser, free);
        harp_parser_state_delete(state);

        return -1;
    }
    if (state->hasError) {
        fprintf(stderr, "Parser error: %s\n", state->error);

        // Cleanup the scanner and parser
        yy_delete_buffer(buf, scanner);
        yylex_destroy(scanner);
        ParseFree(operationParser, free);
        harp_parser_state_delete(state);

        return -1;
    }

    for(i = 0; i < state->result->num_operations; i++)
    {
        harp_operation_print_debug(state->result->operation[i], printf);
    }

    if (harp_program_copy(state->result, new_program) != 0)
    {
        return -1;
    }

    // Cleanup the scanner and parser
    yy_delete_buffer(buf, scanner);
    yylex_destroy(scanner);
    ParseFree(operationParser, free);
    harp_parser_state_delete(state);

    return 0;
}
