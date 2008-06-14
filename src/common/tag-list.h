/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef TAG_LIST_H
#define TAG_LIST_H

#include <stdint.h>
#include "ns3/type-id.h"
#include "tag-buffer.h"

namespace ns3 {

struct TagListData;

/**
 * \ingroup packet
 *
 * \brief keep track of the tags stored in a packet.
 *
 * This class is mostly private to the Packet implementation and users
 * should never have to access it directly.
 *
 * \internal
 * The implementation of this class is a bit tricky so, there are a couple
 * of things to keep in mind here:
 *
 *   - it stores all tags in a single byte buffer: each tag is stored
 *     as 4 32bit integers (TypeId, tag data size, start, end) followed 
 *     by the tag data as generated by Tag::Serialize.
 *
 *   - the struct TagListData structure which contains the tag byte buffer
 *     is shared and, thus, reference-counted. This data structure is unshared
 *     as-needed to emulate COW semantics.
 *
 *   - each tag tags a unique set of bytes identified by the pair of offsets 
 *     (start,end). These offsets are provided by Buffer::GetCurrentStartOffset
 *     and Buffer::GetCurrentEndOffset which means that they are relative to 
 *     the start of the 'virtual byte buffer' as explained in the documentation
 *     for the ns3::Buffer class. Whenever the origin of the offset of the Buffer
 *     instance associated to this TagList instance changes, the Buffer class
 *     reports this to its container Packet class as a bool return value
 *     in Buffer::AddAtStart and Buffer::AddAtEnd. In both cases, when this happens
 *     the Packet class calls TagList::AddAtEnd and TagList::AddAtStart to update
 *     the byte offsets of each tag in the TagList.
 *
 *   - whenever bytes are removed from the packet byte buffer, the TagList offsets 
 *     are never updated because we rely on the fact that they will be updated in
 *     either the next call to Packet::AddHeader or Packet::AddTrailer or when
 *     the user iterates the tag list with Packet::GetTagIterator and 
 *     TagIterator::Next.
 */
class TagList
{
public:

  class Iterator
  {
  public:
    struct Item 
    {
      TypeId tid;
      uint32_t size;
      uint32_t start;
      uint32_t end;
      TagBuffer buf;
      Item (TagBuffer buf);
    private:
      friend class TagList;
      friend class TagList::Iterator;
    };
    bool HasNext (void) const;
    struct TagList::Iterator::Item Next (void);
    uint32_t GetOffsetStart (void) const;
  private:
    friend class TagList;
    Iterator (uint8_t *start, uint8_t *end, uint32_t offsetStart, uint32_t offsetEnd);
    void PrepareForNext (void);
    uint8_t *m_current;
    uint8_t *m_end;
    uint32_t m_offsetStart;
    uint32_t m_offsetEnd;
    uint32_t m_nextTid;
    uint32_t m_nextSize;
    uint32_t m_nextStart;
    uint32_t m_nextEnd;
  };

  TagList ();
  TagList (const TagList &o);
  TagList &operator = (const TagList &o);
  ~TagList ();

  /**
   * \param tid the typeid of the tag added
   * \param bufferSize the size of the tag when its serialization will 
   *        be completed. Typically, the return value of Tag::GetSerializedSize
   * \param start offset which uniquely identifies the first byte tagged by this tag.
   * \param end offset which uniquely identifies the last byte tagged by this tag.
   * \returns a buffer which can be used to write the tag data.     
   *
   * 
   */
  TagBuffer Add (TypeId tid, uint32_t bufferSize, uint32_t start, uint32_t end);

  /**
   * \param o the other list of tags to aggregate.
   *
   * Aggregate the two lists of tags.
   */
  void Add (const TagList &o);

  void RemoveAll (void);

  /**
   * \param offsetStart the offset which uniquely identifies the first data byte 
   *        present in the byte buffer associated to this TagList.
   * \param offsetEnd the offset which uniquely identifies the last data byte 
   *        present in the byte buffer associated to this TagList.
   * \returns an iterator
   *
   * The returned iterator will allow you to loop through the set of tags present
   * in this list: the boundaries of each tag as reported by their start and
   * end offsets will be included within the input offsetStart and offsetEnd.
   */
  TagList::Iterator Begin (uint32_t offsetStart, uint32_t offsetEnd) const;

  /**
   * Adjust the offsets stored internally by the adjustment delta and
   * make sure that all offsets are smaller than appendOffset which represents
   * the location where new bytes have been added to the byte buffer.
   */
  void AddAtEnd (int32_t adjustment, uint32_t appendOffset);
  /**
   * Adjust the offsets stored internally by the adjustment delta and
   * make sure that all offsets are bigger than prependOffset which represents
   * the location where new bytes have been added to the byte buffer.
   */
  void AddAtStart (int32_t adjustment, uint32_t prependOffset);

private:
  bool IsDirtyAtEnd (uint32_t appendOffset);
  bool IsDirtyAtStart (uint32_t prependOffset);

  struct TagListData *Allocate (uint32_t size);
  void Deallocate (struct TagListData *data);

  uint16_t m_used;
  struct TagListData *m_data;
};

} // namespace ns3

#endif /* TAG_LIST_H */