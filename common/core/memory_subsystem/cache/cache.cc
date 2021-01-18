#include "simulator.h"
#include "cache.h"
#include "log.h"

// Cache class
// constructors/destructors
Cache::Cache(
   String name,
   String cfgname,
   core_id_t core_id,
   UInt32 num_sets,
   UInt32 associativity,
   UInt32 cache_block_size,
   String replacement_policy,
   cache_t cache_type,
   hash_t hash,
   FaultInjector *fault_injector,
   AddressHomeLookup *ahl 
   )
:
   CacheBase(name, num_sets, associativity, cache_block_size, hash, ahl),
   m_enabled(false),
   m_num_accesses(0),
   m_num_hits(0),
   m_cache_type(cache_type),
   m_fault_injector(fault_injector)
{
   m_set_info = CacheSet::createCacheSetInfo(name, cfgname, core_id, replacement_policy, m_associativity);
   m_sets = new CacheSet*[m_num_sets];
   for (UInt32 i = 0; i < m_num_sets; i++)
   {
      m_sets[i] = CacheSet::createCacheSet(cfgname, core_id, replacement_policy, m_cache_type, m_associativity, m_blocksize, m_set_info);
   }

   #ifdef ENABLE_SET_USAGE_HIST
   m_set_usage_hist = new UInt64[m_num_sets];
   for (UInt32 i = 0; i < m_num_sets; i++)
      m_set_usage_hist[i] = 0;
   #endif
}

Cache::~Cache()
{
   #ifdef ENABLE_SET_USAGE_HIST
   printf("Cache %s set usage:", m_name.c_str());
   for (SInt32 i = 0; i < (SInt32) m_num_sets; i++)
      printf(" %" PRId64, m_set_usage_hist[i]);
   printf("\n");
   delete [] m_set_usage_hist;
   #endif

   if (m_set_info)
      delete m_set_info;

   for (SInt32 i = 0; i < (SInt32) m_num_sets; i++)
      delete m_sets[i];
   delete [] m_sets;
}

Lock&
Cache::getSetLock(IntPtr addr)
{
   IntPtr tag;
   UInt32 set_index;

   splitAddress(addr, tag, set_index);
   assert(set_index < m_num_sets);

   return m_sets[set_index]->getLock();
}

bool
Cache::invalidateSingleLine(IntPtr addr)
{
   IntPtr tag;
   UInt32 set_index;

   splitAddress(addr, tag, set_index);
   assert(set_index < m_num_sets);

   return m_sets[set_index]->invalidate(tag);
}

CacheBlockInfo*
Cache::accessSingleLine(IntPtr addr, access_t access_type,
      Byte* buff, UInt32 bytes, SubsecondTime now, bool update_replacement)
{
   //assert((buff == NULL) == (bytes == 0));

   IntPtr tag;
   UInt32 set_index;
   UInt32 line_index = -1;
   UInt32 block_offset;

   splitAddress(addr, tag, set_index, block_offset);

   CacheSet* set = m_sets[set_index];
   CacheBlockInfo* cache_block_info = set->find(tag, &line_index);

   if (cache_block_info == NULL)
      return NULL;

   if (access_type == LOAD)
   {
      // NOTE: assumes error occurs in memory. If we want to model bus errors, insert the error into buff instead
      if (m_fault_injector)
         m_fault_injector->preRead(addr, set_index * m_associativity + line_index, bytes, (Byte*)m_sets[set_index]->getDataPtr(line_index, block_offset), now);

      set->read_line(line_index, block_offset, buff, bytes, update_replacement);
   }
   else
   {
      set->write_line(line_index, block_offset, buff, bytes, update_replacement);

      // NOTE: assumes error occurs in memory. If we want to model bus errors, insert the error into buff instead
      if (m_fault_injector)
         m_fault_injector->postWrite(addr, set_index * m_associativity + line_index, bytes, (Byte*)m_sets[set_index]->getDataPtr(line_index, block_offset), now);
   }

   return cache_block_info;
}

void
Cache::insertSingleLine(IntPtr addr, Byte* fill_buff,
      bool* eviction, IntPtr* evict_addr,
      CacheBlockInfo* evict_block_info, Byte* evict_buff,
      SubsecondTime now, CacheCntlr *cntlr)
{
   IntPtr tag;
   UInt32 set_index;
   splitAddress(addr, tag, set_index);

   CacheBlockInfo* cache_block_info = CacheBlockInfo::create(m_cache_type);
   cache_block_info->setTag(tag);

   m_sets[set_index]->insert(cache_block_info, fill_buff,
         eviction, evict_block_info, evict_buff, cntlr);
   *evict_addr = tagToAddress(evict_block_info->getTag());

   if (m_fault_injector) {
      // NOTE: no callback is generated for read of evicted data
      UInt32 line_index = -1;
      __attribute__((unused)) CacheBlockInfo* res = m_sets[set_index]->find(tag, &line_index);
      LOG_ASSERT_ERROR(res != NULL, "Inserted line no longer there?");

      m_fault_injector->postWrite(addr, set_index * m_associativity + line_index, m_sets[set_index]->getBlockSize(), (Byte*)m_sets[set_index]->getDataPtr(line_index, 0), now);
   }

   #ifdef ENABLE_SET_USAGE_HIST
   ++m_set_usage_hist[set_index];
   #endif

   delete cache_block_info;
}


// Single line cache access at addr
CacheBlockInfo*
Cache::peekSingleLine(IntPtr addr)
{
   IntPtr tag;
   UInt32 set_index;
   splitAddress(addr, tag, set_index);

   return m_sets[set_index]->find(tag);
}

void
Cache::updateCounters(bool cache_hit)
{
   if (m_enabled)
   {
      m_num_accesses ++;
      if (cache_hit)
         m_num_hits ++;
   }
}

void
Cache::updateHits(Core::mem_op_t mem_op_type, UInt64 hits)
{
   if (m_enabled)
   {
      m_num_accesses += hits;
      m_num_hits += hits;
   }
}

UInt32
Cache::getBlockIndex(IntPtr addr)
{
	IntPtr tag;
	UInt32 set_index;
	UInt32 BlockIndex;
	
	splitAddress(addr, tag, set_index);
	
	BlockIndex = m_sets[set_index]->getBlockIndexForGivenTag(tag);
   assert(BlockIndex < m_associativity);

   if(BlockIndex >= m_associativity)
   {
      printf("BLOCK NUMBER ERROR \n");
   }

	return BlockIndex;
}

UInt32
Cache::getSetIndex(IntPtr addr)
{
	IntPtr tag;
	UInt32 set_index;
	UInt32 BlockIndex;
	
	splitAddress(addr, tag, set_index);
	assert(set_index < m_num_sets);

   if(set_index >= m_num_sets)
   {
      printf("SET NUMBER ERROR \n");
   }
	
	return set_index;
}


void
Cache::updateLSC(UInt32 setNum, UInt32 lineNum)
{
   
   // if(setNum == 6435)
   //    toDebug();
   // printf("Number of sets = %d \n", m_num_sets);
   // printf("Line Number: %d , Set Number: %d \n", lineNum, setNum);
   // if(setNum<m_num_sets)
   // {
      m_sets[setNum]->m_LSC[lineNum]++;
      // printf("Counter Value : %d \n", m_sets[setNum]->m_LSC[lineNum]);
   // }
   UInt32 minSRAMLSC=m_sets[setNum]->m_LSC[lineNum];
   UInt32 minSRAMLSClineNum = -1;
   if(lineNum>=4)
   {
      if(m_sets[setNum]->m_LSC[lineNum]>=3)
      {
         // printf("LSC greater than 3, LSC: %d, Line Number: %d , Set Number: %d \n", m_sets[setNum]->m_LSC[lineNum], lineNum, setNum);
         for(int i=0; i<4; i++)
         {
            if(m_sets[setNum]->m_LSC[i]<minSRAMLSC)
            {
               minSRAMLSC = m_sets[setNum]->m_LSC[i];
               minSRAMLSClineNum = i;
            }
         }
         if(minSRAMLSClineNum!=-1)
         {
            // printf("SRAM with minimum LSC, LSC: %d, Line Number: %d , Set Number: %d \n", m_sets[setNum]->m_LSC[minSRAMLSC], minSRAMLSClineNum, setNum);
            Byte bytes = 8;
            Byte *in1, *in2, *out1, *out2;
            IntPtr tag1, tag2;
            CacheBlockInfo *info1;
            CacheBlockInfo *info2;
            
            for(UInt32 offset = 0; offset<64; offset+=8)
            {
               m_sets[setNum]->read_line(minSRAMLSClineNum, offset, out1, bytes, false);
               m_sets[setNum]->read_line(lineNum, offset, out2, bytes, false);
               m_sets[setNum]->write_line(minSRAMLSClineNum, offset, out2, bytes, false);
               m_sets[setNum]->write_line(lineNum, offset, out1, bytes, false);
            }
            info1 = m_sets[setNum]->m_cache_block_info_array[minSRAMLSClineNum];
            info2 = (m_sets[setNum]->m_cache_block_info_array[lineNum]);
            // printf("tag1; %d, tag2:%d \n", tag1, tag2);
            m_sets[setNum]->m_cache_block_info_array[minSRAMLSClineNum] = info2;
            m_sets[setNum]->m_cache_block_info_array[lineNum] = info1;
            // printf("NEW tag1; %d, tag2:%d \n", m_sets[setNum]->m_cache_block_info_array[minSRAMLSClineNum]->m_tag, m_sets[setNum]->m_cache_block_info_array[lineNum]->m_tag);

         }
         for(int i=0; i<16; i++)
         {
            m_sets[setNum]->m_LSC[i]=0;
         }

      }
   }
}

// void Cache::toDebug()
// {}