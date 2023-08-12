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
   struct configuration* config;

   size_t cache_size = 0;

   config = (struct configuration*)shmem;

   // first of all, allocate the overall cache structure
   config->query_cache_max_size = 256 * 1024;
   cache_size = config->query_cache_max_size;

   pgagroal_log_info("Query cache initialised");

   if (pgagroal_create_shared_memory(cache_size, config->hugepage, (void*) &cache))
   {
      goto error;
   }

   memset(cache, 0, cache_size);
   cache->size = cache_size;
   // initalizing the hash table
   struct hashTable* Table = NULL;
   cache->table = Table;
   atomic_init(&cache->lock, STATE_FREE);

   // success! do the memory swap
   *p_shmem = cache;
   *p_size = cache_size;
   return 0;

error:
   // disable query caching
   config->query_cache_max_size = 0;
   pgagroal_log_error("Cannot allocate shared memory for the Query cache!");
   *p_size = 0;
   *p_shmem = NULL;

   return 1;
}
struct hashTable*
pgagroal_query_cache_get(struct hashTable** Table, void* key)
{
   struct hashTable* s;
   HASH_FIND_PTR(*Table, &key, s);
   if (s == NULL)
   {
      return NULL;
   }
   return s;
}

int
pgagroal_query_cache_invalidate(struct hashTable** Table, void* key)
{
   struct hashTable* s;
   HASH_FIND_PTR(*Table, &key, s);
   if (s == NULL)
   {
      return 0;
   }
   HASH_DEL(*Table, s);
   free(s);
   return 1;
}

int
pgagroal_query_cache_update(struct hashTable** Table, void* key, char* data)
{
   struct hashTable* s;
   HASH_FIND_PTR(*Table, &key, s);
   if (s == NULL)
   {
      return 0;
   }
   s->data = data;
   return 1;
}

int
pgagroal_query_cache_add(struct hashTable** Table, char* data, void* key)
{
   struct hashTable* s;
   HASH_FIND_PTR(*Table, &key, s);
   if (s != NULL)
   {
      return 0;
   }
   s = (struct hashTable*)malloc(sizeof(struct hashTable));
   s->key = key;
   s->data = data;
   HASH_ADD_PTR(*Table, key, s);

   return 1;
}

int
pgagroal_query_cache_clear(struct hashTable** Table)
{
   struct hashTable* current_item, * tmp;
   HASH_ITER(hh, *Table, current_item, tmp)
   {
      HASH_DEL(*Table, current_item);
      free(current_item);
   }
   return 1;
}
void
pgagroal_query_cache_test(void)
{
   struct query_cache* cache;

   cache = (struct query_cache*)query_cache_shmem;

   char* key = "key";
   char* nKey = "newKey";
   pgagroal_log_info("Add cache entry with key: key and data: data");
   int x = pgagroal_query_cache_add(&(cache->table), "data", key);
   if (x == 0)
   {
      pgagroal_log_info("Key already exists");
   }
   else
   {
      pgagroal_log_info("Key added");
   }
   pgagroal_log_info("Add cache entry with key: key and data: data");
   x = pgagroal_query_cache_add(&(cache->table), "data", key);
   if (x == 0)
   {
      pgagroal_log_info("Key already exists");
   }
   else
   {
      pgagroal_log_info("Key added");
   }
   pgagroal_log_info("Get cache entry with key: key");
   struct hashTable* resp = pgagroal_query_cache_get(&(cache->table), key);
   if (resp != NULL)
   {
      pgagroal_log_info("resp: %s", resp->data);
   }
   else
   {
      pgagroal_log_info("Key not found in cache");
   }
   pgagroal_log_info("Update cache entry with key: key and data: new data");
   pgagroal_query_cache_update(&(cache->table), key, "new data");
   resp = pgagroal_query_cache_get(&(cache->table), key);
   if (resp)
   {
      pgagroal_log_info("resp: %s", resp->data);
   }
   else
   {
      pgagroal_log_info("Key not found in cache");
   }
   pgagroal_log_info("Invalidate cache entry with key: key");
   pgagroal_query_cache_invalidate(&(cache->table), key);
   pgagroal_log_info("Get cache entry with key: key");
   resp = pgagroal_query_cache_get(&(cache->table), key);
   if (resp)
   {
      pgagroal_log_info("resp: %s", resp->data);
   }
   else
   {
      pgagroal_log_info("Key not found in cache");
   }
   pgagroal_log_info("Add cache entry with key: key and data: data");
   pgagroal_query_cache_add(&(cache->table), "data", key);
   pgagroal_log_info("Add cache entry with key: newKey and data: new-data");
   pgagroal_query_cache_add(&(cache->table), "new-data", nKey);
   pgagroal_log_info("Get cache entry with key: newKey");
   resp = pgagroal_query_cache_get(&(cache->table), nKey);
   if (resp)
   {
      pgagroal_log_info("resp: %s", resp->data);
   }
   else
   {
      pgagroal_log_info("Key not found in cache");
   }
   pgagroal_log_info("Clear cache");
   pgagroal_query_cache_clear(&(cache->table));
   pgagroal_log_info("Get cache entry with key: key");

   resp = pgagroal_query_cache_get(&(cache->table), nKey);
   if (resp)
   {
      pgagroal_log_info("resp: %s", resp->data);
   }
   else
   {
      pgagroal_log_info("Key not found in cache");
   }
   pgagroal_log_info("Get cache entry with key: newKey");
   resp = pgagroal_query_cache_get(&(cache->table), key);
   if (resp)
   {
      pgagroal_log_info("resp: %s", resp->data);
   }
   else
   {
      pgagroal_log_info("Key not found in cache");
   }

}
