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
#include <pgagroal.h>
#include <logging.h>
#include <memory.h>
#include <message.h>
#include <network.h>
#include <query_cache.h>
#include <utils.h>
#include <shmem.h>
#include <uthash.h>

/* system */
#include <ev.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

int
pgagroal_query_cache_init(size_t* p_size, void** p_shmem)
{
   struct query_cache* cache;
   // initalizing the hash table
   struct configuration* config;

   size_t cache_size = 0;
   size_t struct_size = 0;

   config = (struct configuration*)shmem;

   // first of all, allocate the overall cache structure
   config->query_cache_max_size = 256 * 1024;
   cache_size = config->query_cache_max_size;
   struct_size = sizeof(struct query_cache);

   if (pgagroal_create_shared_memory(struct_size + cache_size, config->hugepage, (void*) &cache))
   {
      goto error;
   }

   memset(cache, 0, struct_size + cache_size);
   cache->size = cache_size;
   struct hashTable* Table = NULL;
   cache->table = Table;
   atomic_init(&cache->lock, STATE_FREE);

   // success! do the memory swap
   *p_shmem = cache;
   *p_size = cache_size + struct_size;
   return 0;

error:
   // disable query caching
   config->query_cache_max_size = config->metrics_cache_max_size = 0;
   pgagroal_log_error("Cannot allocate shared memory for the Query cache!");
   *p_size = 0;
   *p_shmem = NULL;

   return 1;
}
//get the query cache
struct hashTable*
pgagroal_query_cache_get(struct hashTable* Table, char key)
{
   struct hashTable* s;
   HASH_FIND_INT(Table, &key, s);
   return s;
}

//invalidate the query cache
int
pgagroal_query_cache_invalidate(struct hashTable* Table, char key)
{
   struct hashTable* s;
   HASH_FIND_INT(Table, &key, s);
   if (s == NULL)
   {
      return 0;
   }
   HASH_DEL(Table, s);
   free(s);
   return 1;
}

//update the query cache
int
pgagroal_query_cache_update(struct hashTable* Table, char key, char data)
{
   struct hashTable* s;
   HASH_FIND_INT(Table, &key, s);
   if (s == NULL)
   {
      return 0;
   }
   s->data = data;
   return 1;
}

int
//add a new query to the cache
pgagroal_query_cache_add(struct hashTable* Table, char data, char key)
{
   struct hashTable* obj = (struct hashTable*)malloc(sizeof(struct hashTable));
   obj->key = key;
   obj->data = data;

   HASH_ADD_INT(Table, key, obj);
   return 1;
}

//Clear the entire hashtable and free the memeory
int
pgagroal_query_cache_clear(struct hashTable* Table)
{
   struct hashTable* current_item, * tmp;
   HASH_ITER(hh, Table, current_item, tmp)
   {
      HASH_DEL(Table, current_item);
      free(current_item);
   }
   return 1;
}
