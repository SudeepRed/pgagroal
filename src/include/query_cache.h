/*
 * Copyright (C) 2023 Red Hat
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list
 * of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or other
 * materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may
 * be used to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/*

variables:
    - query_cache: map<query, result>
    - query_cache_size: int
    - query_cache_max_size: int


add init method: pgagroal_init_prometheus_cache
invalidate method: metrics_cache_invalidate
update method: metrics_cache_append
get method: (impletent on ur own)

*/
#include <stdlib.h>

/**
 * Allocates, for the first time, the Query cache.
 *
 * The cache structure, as well as its dynamically sized payload,
 * are created as shared memory chunks.
 *
 * Assumes the shared memory for the cofiguration is already set.
 *
 * The cache will be allocated as soon as this method is invoked,
 * even if the cache has not been configured at all!
 *
 * If the memory cannot be allocated, the function issues errors
 * in the logs and disables the caching machinaery.
 *
 * @param p_size a pointer to where to store the size of
 * allocated chunk of memory
 * @param p_shmem the pointer to the pointer at which the allocated chunk
 * of shared memory is going to be inserted
 *
 * @return 0 on success
 */
int
pgagroal_init_query_cache(size_t* p_size, void** p_shmem);

int //get the query cache
pgagroal_query_cache_get(void* p_shmem, void* p_query, size_t p_query_size, void** p_result, size_t* p_result_size);

int //invalidate the query cache
pgagroal_query_cache_invalidate(void* p_shmem, void* p_query, size_t p_query_size);

int //update the query cache
pgagroal_query_cache_update(void* p_shmem, void* p_query, size_t p_query_size, void* p_result, size_t p_result_size);

int //add a new query to the cache
pgagroal_query_cache_add(void* p_shmem);

int //define a cache invalidator function
pgagroal_query_cache_clear(void* p_shmem, void* p_query, size_t p_query_size);

