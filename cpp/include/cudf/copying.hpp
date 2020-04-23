/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <cudf/column/column_view.hpp>
#include <cudf/scalar/scalar.hpp>
#include <cudf/table/table.hpp>
#include <cudf/types.hpp>

#include <memory>

namespace cudf {
namespace experimental {
/**
 * @brief Gathers the specified rows (including null values) of a set of columns.
 *
 * Gathers the rows of the source columns according to `gather_map` such that row "i"
 * in the resulting table's columns will contain row "gather_map[i]" from the source columns.
 * The number of rows in the result table will be equal to the number of elements in
 * `gather_map`.
 *
 * A negative value `i` in the `gather_map` is interpreted as `i+n`, where
 * `n` is the number of rows in the `source_table`.
 *
 * For dictionary columns, the keys column component is copied and not trimmed
 * if the gather results in abandoned key elements.
 *
 * @throws `cudf::logic_error` if `check_bounds == true` and an index exists in
 * `gather_map` outside the range `[-n, n)`, where `n` is the number of rows in
 * the source table. If `check_bounds == false`, the behavior is undefined.
 *
 * @param[in] source_table The input columns whose rows will be gathered
 * @param[in] gather_map View into a non-nullable column of integral indices that maps the
 * rows in the source columns to rows in the destination columns.
 * @param[in] check_bounds Optionally perform bounds checking on the values
 * of `gather_map` and throw an error if any of its values are out of bounds.
 * @param[in] mr The resource to use for all allocations
 * @return std::unique_ptr<table> Result of the gather
 */
std::unique_ptr<table> gather(
  table_view const& source_table,
  column_view const& gather_map,
  bool check_bounds                   = false,
  rmm::mr::device_memory_resource* mr = rmm::mr::get_default_resource());

/**
 * @brief Scatters the rows of the source table into a copy of the target table
 * according to a scatter map.
 *
 * Scatters values from the source table into the target table out-of-place,
 * returning a "destination table". The scatter is performed according to a
 * scatter map such that row `scatter_map[i]` of the destination table gets row
 * `i` of the source table. All other rows of the destination table equal
 * corresponding rows of the target table.
 *
 * The number of columns in source must match the number of columns in target
 * and their corresponding datatypes must be the same.
 *
 * If the same index appears more than once in the scatter map, the result is
 * undefined.
 *
 * A negative value `i` in the `scatter_map` is interpreted as `i+n`, where `n`
 * is the number of rows in the `target` table.
 *
 * @throws `cudf::logic_error` if `check_bounds == true` and an index exists in
 * `scatter_map` outside the range `[-n, n)`, where `n` is the number of rows in
 * the target table. If `check_bounds == false`, the behavior is undefined.
 *
 * @param source The input columns containing values to be scattered into the
 * target columns
 * @param scatter_map A non-nullable column of integral indices that maps the
 * rows in the source table to rows in the target table. The size must be equal
 * to or less than the number of elements in the source columns.
 * @param target The set of columns into which values from the source_table
 * are to be scattered
 * @param check_bounds Optionally perform bounds checking on the values of
 * `scatter_map` and throw an error if any of its values are out of bounds.
 * @param mr The resource to use for all allocations
 * @return Result of scattering values from source to target
 *---------------------------------------------------------------------------**/
std::unique_ptr<table> scatter(
  table_view const& source,
  column_view const& scatter_map,
  table_view const& target,
  bool check_bounds                   = false,
  rmm::mr::device_memory_resource* mr = rmm::mr::get_default_resource());

/**
 * @brief Scatters a row of scalar values into a copy of the target table
 * according to a scatter map.
 *
 * Scatters values from the source row into the target table out-of-place,
 * returning a "destination table". The scatter is performed according to a
 * scatter map such that row `scatter_map[i]` of the destination table is
 * replaced by the source row. All other rows of the destination table equal
 * corresponding rows of the target table.
 *
 * The number of elements in source must match the number of columns in target
 * and their corresponding datatypes must be the same.
 *
 * If the same index appears more than once in the scatter map, the result is
 * undefined.
 *
 * @throws `cudf::logic_error` if `check_bounds == true` and an index exists in
 * `scatter_map` outside the range `[-n, n)`, where `n` is the number of rows in
 * the target table. If `check_bounds == false`, the behavior is undefined.
 *
 * @param source The input scalars containing values to be scattered into the
 * target columns
 * @param indices A non-nullable column of integral indices that indicate
 * the rows in the target table to be replaced by source.
 * @param target The set of columns into which values from the source_table
 * are to be scattered
 * @param check_bounds Optionally perform bounds checking on the values of
 * `scatter_map` and throw an error if any of its values are out of bounds.
 * @param mr The resource to use for all allocations
 * @return Result of scattering values from source to target
 *---------------------------------------------------------------------------**/
std::unique_ptr<table> scatter(
  std::vector<std::unique_ptr<scalar>> const& source,
  column_view const& indices,
  table_view const& target,
  bool check_bounds                   = false,
  rmm::mr::device_memory_resource* mr = rmm::mr::get_default_resource());

/** ---------------------------------------------------------------------------*
 * @brief Indicates when to allocate a mask, based on an existing mask.
 * ---------------------------------------------------------------------------**/
enum class mask_allocation_policy {
  NEVER,   ///< Do not allocate a null mask, regardless of input
  RETAIN,  ///< Allocate a null mask if the input contains one
  ALWAYS   ///< Allocate a null mask, regardless of input
};

/*
 * Initializes and returns an empty column of the same type as the `input`.
 *
 * @param[in] input Immutable view of input column to emulate
 * @return std::unique_ptr<column> An empty column of same type as `input`
 */
std::unique_ptr<column> empty_like(column_view const& input);

/**
 * @brief Creates an uninitialized new column of the same size and type as the `input`.
 * Supports only fixed-width types.
 *
 * @param[in] input Immutable view of input column to emulate
 * @param[in] mask_alloc Optional, Policy for allocating null mask. Defaults to RETAIN.
 * @param[in] mr Optional, The resource to use for all allocations
 * @return std::unique_ptr<column> A column with sufficient uninitialized capacity to hold the same
 * number of elements as `input` of the same type as `input.type()`
 */
std::unique_ptr<column> allocate_like(
  column_view const& input,
  mask_allocation_policy mask_alloc   = mask_allocation_policy::RETAIN,
  rmm::mr::device_memory_resource* mr = rmm::mr::get_default_resource());

/**
 * @brief Creates an uninitialized new column of the specified size and same type as the `input`.
 * Supports only fixed-width types.
 *
 * @param[in] input Immutable view of input column to emulate
 * @param[in] size The desired number of elements that the new column should have capacity for
 * @param[in] mask_alloc Optional, Policy for allocating null mask. Defaults to RETAIN.
 * @param[in] mr Optional, The resource to use for all allocations
 * @return std::unique_ptr<column> A column with sufficient uninitialized capacity to hold the
 * specified number of elements as `input` of the same type as `input.type()`
 */
std::unique_ptr<column> allocate_like(
  column_view const& input,
  size_type size,
  mask_allocation_policy mask_alloc   = mask_allocation_policy::RETAIN,
  rmm::mr::device_memory_resource* mr = rmm::mr::get_default_resource());

/**
 * @brief Creates a table of empty columns with the same types as the `input_table`
 *
 * Creates the `cudf::column` objects, but does not allocate any underlying device
 * memory for the column's data or bitmask.
 *
 * @param[in] input_table Immutable view of input table to emulate
 * @return std::unique_ptr<table> A table of empty columns with the same types as the columns in
 * `input_table`
 */
std::unique_ptr<table> empty_like(table_view const& input_table);

/**
 * @brief Copies a range of elements in-place from one column to another.
 *
 * Overwrites the range of elements in @p target indicated by the indices
 * [@p target_begin, @p target_begin + N) with the elements from @p source
 * indicated by the indices [@p source_begin, @p source_end) (where N =
 * (@p source_end - @p source_begin)). Use the out-of-place copy function
 * returning std::unique_ptr<column> for uses cases requiring memory
 * reallocation. For example for strings columns and other variable-width types.
 *
 * If @p source and @p target refer to the same elements and the ranges overlap,
 * the behavior is undefined.
 *
 * @throws `cudf::logic_error` if memory reallocation is required (e.g. for
 * variable width types).
 * @throws `cudf::logic_error` for invalid range (if
 * @p source_begin > @p source_end, @p source_begin < 0,
 * @p source_end > @p source.size(), @p target_begin < 0,
 * or @p target_begin + (@p source_end - @p source_begin) > @p target.size()).
 * @throws `cudf::logic_error` if @p target and @p source have different types.
 * @throws `cudf::logic_error` if @p source has null values and @p target is not
 * nullable.
 *
 * @param source The column to copy from
 * @param target The preallocated column to copy into
 * @param source_begin The starting index of the source range (inclusive)
 * @param source_end The index of the last element in the source range
 * (exclusive)
 * @param target_begin The starting index of the target range (inclusive)
 * @return void
 */
void copy_range_in_place(column_view const& source,
                         mutable_column_view& target,
                         size_type source_begin,
                         size_type source_end,
                         size_type target_begin);

/**
 * @brief Copies a range of elements out-of-place from one column to another.
 *
 * Creates a new column as if an in-place copy was performed into @p target.
 * A copy of @p target is created first and then the elements indicated by the
 * indices [@p target_begin, @p target_begin + N) were copied from the elements
 * indicated by the indices [@p source_begin, @p source_end) of @p source
 * (where N = (@p source_end - @p source_begin)). Elements outside the range are
 * copied from @p target into the returned new column target.
 *
 * If @p source and @p target refer to the same elements and the ranges overlap,
 * the behavior is undefined.
 *
 * @throws `cudf::logic_error` for invalid range (if
 * @p source_begin > @p source_end, @p source_begin < 0,
 * @p source_end > @p source.size(), @p target_begin < 0,
 * or @p target_begin + (@p source_end - @p source_begin) > @p target.size()).
 * @throws `cudf::logic_error` if @p target and @p source have different types.
 *
 * @param source The column to copy from inside the range.
 * @param target The column to copy from outside the range.
 * @param source_begin The starting index of the source range (inclusive)
 * @param source_end The index of the last element in the source range
 * (exclusive)
 * @param target_begin The starting index of the target range (inclusive)
 * @param mr Memory resource to allocate the result target column.
 * @return std::unique_ptr<column> The result target column
 */
std::unique_ptr<column> copy_range(
  column_view const& source,
  column_view const& target,
  size_type source_begin,
  size_type source_end,
  size_type target_begin,
  rmm::mr::device_memory_resource* mr = rmm::mr::get_default_resource());

/**
 * @brief Slices a `column_view` into a set of `column_view`s according to a set of indices.
 * The returned views of `input` are constructed from an even number indices where
 * the `i`th returned `column_view` views the elements in `input` indicated by the range
 * `[indices[2*i], indices[(2*i)+1])`.
 *
 * For all `i` it is expected `indices[i] <= input.size()`
 * For all `i%2==0`, it is expected that `indices[i] <= indices[i+1]`
 *
 * @note It is the caller's responsibility to ensure that the returned views
 * do not outlive the viewed device memory.
 *
 * @example:
 * input:   {10, 12, 14, 16, 18, 20, 22, 24, 26, 28}
 * indices: {1, 3, 5, 9, 2, 4, 8, 8}
 * output:  {{12, 14}, {20, 22, 24, 26}, {14, 16}, {}}
 *
 * @throws `cudf::logic_error` if `indices` size is not even.
 * @throws `cudf::logic_error` When the values in the pair are strictly decreasing.
 * @throws `cudf::logic_error` When any of the values in the pair don't belong to
 * the range [0, input.size()).
 *
 * @param input View of column to slice
 * @param indices A vector of indices used to take slices of `input`.
 * @return Vector of views of `input` indicated by the ranges in `indices`.
 */
std::vector<column_view> slice(column_view const& input, std::vector<size_type> const& indices);

/**
 * @brief Slices a `table_view` into a set of `table_view`s according to a set of indices.
 *
 * The returned views of `input` are constructed from an even number indices where
 * the `i`th returned `table_view` views the elements in `input` indicated by the range
 * `[indices[2*i], indices[(2*i)+1])`.
 *
 * For all `i` it is expected `indices[i] <= input.size()`
 * For all `i%2==0`, it is expected that `indices[i] <= indices[i+1]`
 *
 * @note It is the caller's responsibility to ensure that the returned views
 * do not outlive the viewed device memory.
 *
 * @example:
 * input:   [{10, 12, 14, 16, 18, 20, 22, 24, 26, 28},
 *           {50, 52, 54, 56, 58, 60, 62, 64, 66, 68}]
 * indices: {1, 3, 5, 9, 2, 4, 8, 8}
 * output:  [{{12, 14}, {20, 22, 24, 26}, {14, 16}, {}},
 *           {{52, 54}, {60, 22, 24, 26}, {14, 16}, {}}]
 *
 * @throws `cudf::logic_error` if `indices` size is not even.
 * @throws `cudf::logic_error` When the values in the pair are strictly decreasing.
 * @throws `cudf::logic_error` When any of the values in the pair don't belong to
 * the range [0, input.size()).
 *
 * @param input View of table to slice
 * @param indices A vector of indices used to take slices of `input`.
 * @return Vector of views of `input` indicated by the ranges in `indices`.
 */
std::vector<table_view> slice(table_view const& input, std::vector<size_type> const& indices);

/**
 * @brief Splits a `column_view` into a set of `column_view`s according to a set of indices
 * derived from expected splits.
 *
 * The returned view's of `input` are constructed from vector of splits, which indicates
 * where the split should occur. The `i`th returned `column_view` is sliced as
 * `[0, splits[i])` if `i`=0, else `[splits[i], input.size())` if `i` is the last view and
 * `[splits[i-1], splits[i]]` otherwise.
 *
 * For all `i` it is expected `splits[i] <= splits[i+1] <= input.size()`
 * For a `splits` size N, there will always be N+1 splits in the output
 *
 * @note It is the caller's responsibility to ensure that the returned views
 * do not outlive the viewed device memory.
 *
 * Example:
 * input:   {10, 12, 14, 16, 18, 20, 22, 24, 26, 28}
 * splits:  {2, 5, 9}
 * output:  {{10, 12}, {14, 16, 18}, {20, 22, 24, 26}, {28}}
 *
 * @throws `cudf::logic_error` if `splits` has end index > size of `input`.
 * @throws `cudf::logic_error` When the value in `splits` is not in the range [0, input.size()).
 * @throws `cudf::logic_error` When the values in the `splits` are 'strictly decreasing'.
 *
 * @param input View of column to split
 * @param splits A vector of indices where the view will be split
 * @return The set of requested views of `input` indicated by the `splits`.
 */
std::vector<column_view> split(column_view const& input, std::vector<size_type> const& splits);

/**
 * @brief Splits a `table_view` into a set of `table_view`s according to a set of indices
 * derived from expected splits.
 *
 * The returned views of `input` are constructed from vector of splits, which indicates
 * where the split should occur. The `i`th returned `table_view` is sliced as
 * `[0, splits[i])` if `i`=0, else `[splits[i], input.size())` if `i` is the last view and
 * `[splits[i-1], splits[i]]` otherwise.
 *
 * For all `i` it is expected `splits[i] <= splits[i+1] <= input.size()`
 * For a `splits` size N, there will always be N+1 splits in the output
 *
 * @note It is the caller's responsibility to ensure that the returned views
 * do not outlive the viewed device memory.
 *
 * Example:
 * input:   [{10, 12, 14, 16, 18, 20, 22, 24, 26, 28},
 *           {50, 52, 54, 56, 58, 60, 62, 64, 66, 68}]
 * splits:  {2, 5, 9}
 * output:  [{{10, 12}, {14, 16, 18}, {20, 22, 24, 26}, {28}},
 *           {{50, 52}, {54, 56, 58}, {60, 62, 64, 66}, {68}}]
 *
 *
 * @throws `cudf::logic_error` if `splits` has end index > size of `input`.
 * @throws `cudf::logic_error` When the value in `splits` is not in the range [0, input.size()).
 * @throws `cudf::logic_error` When the values in the `splits` are 'strictly decreasing'.
 *
 * @param input View of a table to split
 * @param splits A vector of indices where the view will be split
 * @return The set of requested views of `input` indicated by the `splits`.
 */
std::vector<table_view> split(table_view const& input, std::vector<size_type> const& splits);

/**
 * @brief The result(s) of a `contiguous_split`
 *
 * Each table_view resulting from a split operation performed by contiguous_split,
 * will be returned wrapped in a `contiguous_split_result`.  The table_view and internal
 * column_views in this struct are not owned by a top level cudf::table or cudf::column.
 * The backing memory is instead owned by the `all_data` field and in one
 * contiguous block.
 *
 * The user is responsible for assuring that the `table` or any derived table_views do
 * not outlive the memory owned by `all_data`
 */
struct contiguous_split_result {
  cudf::table_view table;
  std::unique_ptr<rmm::device_buffer> all_data;
};

/**
 * @brief Performs a deep-copy split of a `table_view` into a set of `table_view`s into a single
 * contiguous block of memory.
 *
 * The memory for the output views is allocated in a single contiguous `rmm::device_buffer` returned
 * in the `contiguous_split_result`. There is no top-level owning table.
 *
 * The returned views of `input` are constructed from a vector of indices, that indicate
 * where each split should occur. The `i`th returned `table_view` is sliced as
 * `[0, splits[i])` if `i`=0, else `[splits[i], input.size())` if `i` is the last view and
 * `[splits[i-1], splits[i]]` otherwise.
 *
 * For all `i` it is expected `splits[i] <= splits[i+1] <= input.size()`
 * For a `splits` size N, there will always be N+1 splits in the output
 *
 * @note It is the caller's responsibility to ensure that the returned views
 * do not outlive the viewed device memory contained in the `all_data` field of the
 * returned contiguous_split_result.
 *
 * Example:
 * input:   [{10, 12, 14, 16, 18, 20, 22, 24, 26, 28},
 *           {50, 52, 54, 56, 58, 60, 62, 64, 66, 68}]
 * splits:  {2, 5, 9}
 * output:  [{{10, 12}, {14, 16, 18}, {20, 22, 24, 26}, {28}},
 *           {{50, 52}, {54, 56, 58}, {60, 62, 64, 66}, {68}}]
 *
 *
 * @throws `cudf::logic_error` if `splits` has end index > size of `input`.
 * @throws `cudf::logic_error` When the value in `splits` is not in the range [0, input.size()).
 * @throws `cudf::logic_error` When the values in the `splits` are 'strictly decreasing'.
 *
 * @param input View of a table to split
 * @param splits A vector of indices where the view will be split
 * @param[in] mr Optional, The resource to use for all returned allocations
 * @param[in] stream Optional CUDA stream on which to execute kernels
 * @return The set of requested views of `input` indicated by the `splits` and the viewed memory
 * buffer.
 */
std::vector<contiguous_split_result> contiguous_split(
  cudf::table_view const& input,
  std::vector<size_type> const& splits,
  rmm::mr::device_memory_resource* mr = rmm::mr::get_default_resource());

/**
 * @brief   Returns a new column, where each element is selected from either @p lhs or
 *          @p rhs based on the value of the corresponding element in @p boolean_mask
 *
 * Selects each element i in the output column from either @p rhs or @p lhs using the following
 * rule: output[i] = (boolean_mask.valid(i) and boolean_mask[i]) ? lhs[i] : rhs[i]
 *
 * @throws cudf::logic_error if lhs and rhs are not of the same type
 * @throws cudf::logic_error if lhs and rhs are not of the same length
 * @throws cudf::logic_error if boolean mask is not of type bool
 * @throws cudf::logic_error if boolean mask is not of the same length as lhs and rhs
 * @param[in] left-hand column_view
 * @param[in] right-hand column_view
 * @param[in] column of `BOOL8` representing "left (true) / right (false)" boolean for each element
 * and null element represents false.
 * @param[in] mr resource for allocating device memory
 *
 * @returns new column with the selected elements
 */
std::unique_ptr<column> copy_if_else(
  column_view const& lhs,
  column_view const& rhs,
  column_view const& boolean_mask,
  rmm::mr::device_memory_resource* mr = rmm::mr::get_default_resource());

/**
 * @brief Creates a new column by shifting all values by an offset.
 *
 * Elements will be determined by `output[idx] = input[idx - offset]`.
 * Some elements in the output may be indeterminable from the input. For those
 * elements, the value will be determined by `fill_values`.
 *
 * Examples
 * -------------------------------------------------
 * input       = [0, 1, 2, 3, 4]
 * offset      = 3
 * fill_values = @
 * return      = [@, @, @, 0, 1]
 * -------------------------------------------------
 * input       = [5, 4, 3, 2, 1]
 * offset      = -2
 * fill_values = 7
 * return      = [3, 2, 1, 7, 7]
 *
 * @note if the input is nullable, the output will be nullable.
 * @note if the fill value is null, the output will be nullable.
 *
 * @param input      Column to be shifted.
 * @param offset     The offset by which to shift the input.
 * @param fill_value Fill value for indeterminable outputs.
 *
 * @throw cudf::logic_error if @p input dtype is not fixed-with.
 * @throw cudf::logic_error if @p fill_value dtype does not match @p input dtype.
 */
std::unique_ptr<column> shift(column_view const& input,
                              size_type offset,
                              scalar const& fill_value,
                              rmm::mr::device_memory_resource* mr = rmm::mr::get_default_resource(),
                              cudaStream_t stream                 = 0);

/*
 * @brief   Returns a new column, where each element is selected from either @p lhs or
 *          @p rhs based on the value of the corresponding element in @p boolean_mask
 *
 * Selects each element i in the output column from either @p rhs or @p lhs using the following
 * rule: output[i] = (boolean_mask.valid(i) and boolean_mask[i]) ? lhs : rhs[i]
 *
 * @throws cudf::logic_error if lhs and rhs are not of the same type
 * @throws cudf::logic_error if boolean mask is not of type bool
 * @throws cudf::logic_error if boolean mask is not of the same length as rhs
 * @param[in] left-hand scalar
 * @param[in] right-hand column_view
 * @param[in] column of `BOOL8` representing "left (true) / right (false)" boolean for each element
 * and null element represents false.
 * @param[in] mr resource for allocating device memory
 *
 * @returns new column with the selected elements
 */
std::unique_ptr<column> copy_if_else(
  scalar const& lhs,
  column_view const& rhs,
  column_view const& boolean_mask,
  rmm::mr::device_memory_resource* mr = rmm::mr::get_default_resource());

/**
 * @brief   Returns a new column, where each element is selected from either @p lhs or
 *          @p rhs based on the value of the corresponding element in @p boolean_mask
 *
 * Selects each element i in the output column from either @p rhs or @p lhs using the following
 * rule: output[i] = (boolean_mask.valid(i) and boolean_mask[i]) ? lhs[i] : rhs
 *
 * @throws cudf::logic_error if lhs and rhs are not of the same type
 * @throws cudf::logic_error if boolean mask is not of type bool
 * @throws cudf::logic_error if boolean mask is not of the same length as lhs
 * @param[in] left-hand column_view
 * @param[in] right-hand scalar
 * @param[in] column of `BOOL8` representing "left (true) / right (false)" boolean for each element
 * and null element represents false.
 * @param[in] mr resource for allocating device memory
 *
 * @returns new column with the selected elements
 */
std::unique_ptr<column> copy_if_else(
  column_view const& lhs,
  scalar const& rhs,
  column_view const& boolean_mask,
  rmm::mr::device_memory_resource* mr = rmm::mr::get_default_resource());

/**
 * @brief   Returns a new column, where each element is selected from either @p lhs or
 *          @p rhs based on the value of the corresponding element in @p boolean_mask
 *
 * Selects each element i in the output column from either @p rhs or @p lhs using the following
 * rule: output[i] = (boolean_mask.valid(i) and boolean_mask[i]) ? lhs : rhs
 *
 * @throws cudf::logic_error if boolean mask is not of type bool
 * @param[in] left-hand scalar
 * @param[in] right-hand scalar
 * @param[in] column of `BOOL8` representing "left (true) / right (false)" boolean for each element
 * and null element represents false.
 * @param[in] mr resource for allocating device memory
 *
 * @returns new column with the selected elements
 */
std::unique_ptr<column> copy_if_else(
  scalar const& lhs,
  scalar const& rhs,
  column_view const& boolean_mask,
  rmm::mr::device_memory_resource* mr = rmm::mr::get_default_resource());

/**
 * @brief Scatters rows from the input table to rows of the output corresponding
 * to true values in a boolean mask.
 *
 * The `i`th row of `input` will be written to the output table at the location
 * of the `i`th true value in `boolean_mask`. All other rows in the output will
 * equal the same row in `target`.
 *
 * `boolean_mask` should have number of `true`s <= number of rows in `input`.
 * If boolean mask is `true`, corresponding value in target is updated with
 * value from corresponding `input` column, else it is left untouched.
 *
 * Example:
 * input: {{1, 5, 6, 8, 9}}
 * boolean_mask: {true, false, false, false, true, true, false, true, true, false}
 * target:       {{   2,     2,     3,     4,    4,     7,    7,    7,    8,    10}}
 *
 * output:       {{   1,     2,     3,     4,    5,     6,    7,    8,    9,    10}}
 *
 * @throw  cudf::logic_error if input.num_columns() != target.num_columns()
 * @throws cudf::logic_error if any `i`th input_column type != `i`th target_column type
 * @throws cudf::logic_error if boolean_mask.type() != bool
 * @throws cudf::logic_error if boolean_mask.size() != target.num_rows()
 * @throws cudf::logic_error if number of `true` in `boolean_mask` > input.num_rows()
 *
 * @param[in] input table_view (set of dense columns) to scatter
 * @param[in] target table_view to modify with scattered values from `input`
 * @param[in] boolean_mask column_view which acts as boolean mask.
 * @param[in] mr Optional, The resource to use for all returned allocations
 *
 * @returns Returns a table by scattering `input` into `target` as per `boolean_mask`.
 */
std::unique_ptr<table> boolean_mask_scatter(
  table_view const& input,
  table_view const& target,
  column_view const& boolean_mask,
  rmm::mr::device_memory_resource* mr = rmm::mr::get_default_resource());

/**
 * @brief Scatters scalar values to rows of the output corresponding
 * to true values in a boolean mask.
 *
 * The `i`th scalar in `input` will be written to all columns of the output
 * table at the location of the `i`th true value in `boolean_mask`.
 * All other rows in the output will equal the same row in `target`.
 *
 * Example:
 * input: {11}
 * boolean_mask: {true, false, false, false, true, true, false, true, true, false}
 * target:       {{   2,     2,     3,     4,    4,     7,    7,    7,    8,    10}}
 *
 * output:       {{   11,    2,     3,     4,   11,    11,    7,   11,   11,    10}}
 *
 * @throw  cudf::logic_error if input.size() != target.num_columns()
 * @throws cudf::logic_error if any `i`th input_scalar type != `i`th target_column type
 * @throws cudf::logic_error if boolean_mask.type() != bool
 * @throws cudf::logic_error if boolean_mask.size() != target.size()
 *
 * @param[in] input scalars to scatter
 * @param[in] target table_view to modify with scattered values from `input`
 * @param[in] boolean_mask column_view which acts as boolean mask.
 * @param[in] mr Optional, The resource to use for all returned allocations
 *
 * @returns Returns a table by scattering `input` into `target` as per `boolean_mask`.
 */
std::unique_ptr<table> boolean_mask_scatter(
  std::vector<std::reference_wrapper<scalar>> const& input,
  table_view const& target,
  column_view const& boolean_mask,
  rmm::mr::device_memory_resource* mr = rmm::mr::get_default_resource());

}  // namespace experimental
}  // namespace cudf
