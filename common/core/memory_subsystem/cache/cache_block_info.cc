#include "cache_block_info.h"
#include "pr_l1_cache_block_info.h"
#include "pr_l2_cache_block_info.h"
#include "shared_cache_block_info.h"
#include "log.h"

const char* CacheBlockInfo::option_names[] =
{
   "prefetch",
   "warmup",
};

const char* CacheBlockInfo::getOptionName(option_t option)
{
   static_assert(CacheBlockInfo::NUM_OPTIONS == sizeof(CacheBlockInfo::option_names) / sizeof(char*), "Not enough values in CacheBlockInfo::option_names");

   if (option < NUM_OPTIONS)
      return option_names[option];
   else
      return "invalid";
}


CacheBlockInfo::CacheBlockInfo(IntPtr tag, CacheState::cstate_t cstate, UInt64 options):
   m_tag(tag),
   m_cstate(cstate),
   m_owner(0),
   m_used(0),
   m_options(options),
   m_dirtyWord(0)
{
}

CacheBlockInfo::~CacheBlockInfo()
{
    //delete [] dirty_word;
}

CacheBlockInfo*
CacheBlockInfo::create(CacheBase::cache_t cache_type)
{
   switch (cache_type)
   {
      case CacheBase::PR_L1_CACHE:
         return new PrL1CacheBlockInfo();

      case CacheBase::PR_L2_CACHE:
         return new PrL2CacheBlockInfo();

      case CacheBase::SHARED_CACHE:
         return new SharedCacheBlockInfo();

      default:
         LOG_PRINT_ERROR("Unrecognized cache type (%u)", cache_type);
         return NULL;
   }
}

void
CacheBlockInfo::invalidate()
{
   m_tag = ~0;
   m_cstate = CacheState::INVALID;
}

void
CacheBlockInfo::clone(CacheBlockInfo* cache_block_info)
{
   m_tag = cache_block_info->getTag();
   m_cstate = cache_block_info->getCState();
   m_owner = cache_block_info->m_owner;
   m_used = cache_block_info->m_used;
   m_options = cache_block_info->m_options;
   m_dirtyWord = cache_block_info->m_dirtyWord;
   
   //dirty_word = new UInt32[8];
   //for(int i =0; i<8; i++)
   //   dirty_word[i] = cache_block_info->dirty_word[i];
}

bool
CacheBlockInfo::updateUsage(UInt32 offset, UInt32 size)
{
   UInt64 first = offset >> BitsUsedOffset,
          last  = (offset + size - 1) >> BitsUsedOffset,
          first_mask = (1ull << first) - 1,
          last_mask = (1ull << (last + 1)) - 1,
          usage_mask = last_mask & ~first_mask;

   return updateUsage(usage_mask);
}

bool
CacheBlockInfo::updateUsage(BitsUsedType used)
{
   bool new_bits_set = used & ~m_used; // Are we setting any bits that were previously unset?
   m_used |= used;                     // Update usage mask
   return new_bits_set;
}

void
CacheBlockInfo::setDirtyBit(UInt32 i)
{
   switch(i)
   {
      case 0: m_dirtyWord = m_dirtyWord | 0b00000001;
               break;
      case 1: m_dirtyWord = m_dirtyWord | 0b00000010;
               break;
      case 2: m_dirtyWord = m_dirtyWord | 0b00000100;
               break;
      case 3: m_dirtyWord = m_dirtyWord | 0b00001000;
               break;
      case 4: m_dirtyWord = m_dirtyWord | 0b00010000;
               break;
      case 5: m_dirtyWord = m_dirtyWord | 0b00100000;
               break;
      case 6: m_dirtyWord = m_dirtyWord | 0b01000000;
               break;
      case 7: m_dirtyWord = m_dirtyWord | 0b10000000;
               break;
      default: m_dirtyWord = 0;
   }
}

void
CacheBlockInfo::resetDirtyBit(UInt32 i)
{
   switch(i)
   {
      case 0: m_dirtyWord = m_dirtyWord & 0b11111110;
               break;
      case 1: m_dirtyWord = m_dirtyWord & 0b11111101;
               break;
      case 2: m_dirtyWord = m_dirtyWord & 0b11111011;
               break;
      case 3: m_dirtyWord = m_dirtyWord & 0b11110111;
               break;
      case 4: m_dirtyWord = m_dirtyWord & 0b11101111;
               break;
      case 5: m_dirtyWord = m_dirtyWord & 0b11011111;
               break;
      case 6: m_dirtyWord = m_dirtyWord & 0b10111111;
               break;
      case 7: m_dirtyWord = m_dirtyWord & 0b01111111;
               break;
      default: m_dirtyWord = 0;
   } 
}

void
CacheBlockInfo::copyDirtyWord(CacheBlockInfo* evict_block_info)
{
   m_dirtyWord = evict_block_info->m_dirtyWord;
}

UInt8
CacheBlockInfo::getDirtyWord()
{
   return m_dirtyWord;
}
