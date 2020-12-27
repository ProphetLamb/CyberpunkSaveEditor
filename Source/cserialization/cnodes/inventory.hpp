#pragma once
#include <iostream>

#include "cserialization/node.hpp"
#include "cserialization/cpnames.hpp"
#include "utils.hpp"

/*
------------------------
NOTES
-----------------------

struct_csav.qwordE0 + 648   is ver1

--------------------------------------
cnt = read(4)

vec<8> a(cnt)
vec<16> b(cnt)

for i in range(cnt)
  c = read(8)
  d = read(4)
  if d:
    break  <- thx IDA for this aweful control flow.. (--') goes to the "while 1"

label_loop <-  

---------
if !d:



---------

while 1
  read_string_by_hashid_if_v193p()  <- question is, can i get a json of names so that i compute the hashes myself ?
  e = read(4)
  if VER1 >= 97:
    f = read(1)
  if VER1 >= 190:
    g = read(2)

*/

struct item_id
{
  namehash nameid;
  uint32_t uk4;
  uint8_t uk1;
  uint16_t uk2;

  friend node_reader& operator<<(node_reader& reader, item_id& iid)
  {
    reader.read((char*)&iid.nameid.as_u64, 8);
    reader.read((char*)&iid.uk4, 4);
    reader.read((char*)&iid.uk1, 1);
    reader.read((char*)&iid.uk2, 2);
    return reader;
  }

  friend node_writer& operator<<(node_writer& writer, item_id& iid)
  {
    writer.write((char*)&iid.nameid.as_u64, 8);
    writer.write((char*)&iid.uk4, 4);
    writer.write((char*)&iid.uk1, 1);
    writer.write((char*)&iid.uk2, 2);
    return writer;
  }

  uint8_t kind() const
  {
    if (uk1 == 1) return 2;
    if (uk1 == 2) return 1;
    if (uk1 == 3) return 0;
    return uk4 != 2 ? 2 : 1;
  }

  std::string name() const
  {
    auto& cpn = cpnames::get();
    return cpn.get_name(nameid);
  }
};

struct itemData
{
  std::shared_ptr<const node_t> raw;
  item_id iid;

  // (8, 4, 1, 2), 1, 4, 8, 4, 4, (8, 4, 1, 2), 1, 4, 8, 1, (8, 4, 1, 2), 1, 4, 8, 1, 4, 8, 4, 4, (8, 4, 1, 2), 1, 4, 8, 1, 4, 8, 4, 4, (8, 4, 1, 2), 1, 4, 8, 1, 4, 8, 4, 4, 4, 8, 4, 4, 
  // (8, 4, 1, 2), 1, 4, 8, 4, 4, (8, 4, 1, 2), 1, 7, 8, 1, 4, 8, 4, 4, 

  bool from_node(const std::shared_ptr<const node_t>& node)
  {
    if (!node)
      return false;
    raw = node;

    node_reader reader(node);
    reader << iid;
    auto kind = iid.kind();

    // serial func at 0x128
    // ikind0 vtbl 0x143244E30 -> 0x143244F58
    // ikind1 vtbl 0x143244A20 -> 0x143244B48
    // ikind2 vtbl 0x143244C28 -> 0x143244D50

    // todo
    return true;
  }

  std::shared_ptr<const node_t> to_node()
  {
    // todo
    return raw;
  }

  std::string name()
  {
    iid.name();
  }
};


struct subinv_t
{
  uint64_t uid = 0;
  std::vector<itemData> items;
};


struct inventory
{
  std::vector<subinv_t> m_subinvs;

  bool from_node(const std::shared_ptr<const node_t>& node)
  {
    if (!node)
      return false;

    node_reader reader(node);

    uint32_t inventory_cnt;
    reader.read((char*)&inventory_cnt, 4);
    m_subinvs.resize(inventory_cnt);

    for (auto& subinv : m_subinvs)
    {
      reader.read((char*)&subinv.uid, 8);

      uint32_t items_cnt;
      reader.read((char*)&items_cnt, 4);

      subinv.items.resize(items_cnt);
      for (auto& entry : subinv.items)
      {
        namehash id;
        reader.read((char*)&id, 7);
        uint64_t uk;
        reader.read((char*)&uk, 8);

        auto item_node = reader.read_child("itemData");
        if (!item_node)
          return false; // todo: don't leave this in this state

        if (!entry.from_node(item_node))
          return false;
      }
    }

    return reader.at_end();
  }

  std::shared_ptr<const node_t> to_node()
  {
    node_writer writer;

    uint32_t inventory_cnt = (uint32_t)m_subinvs.size();
    writer.write((char*)&inventory_cnt, 4);

    for (auto& subinv : m_subinvs)
    {
      writer.write((char*)&subinv.uid, 8);

      uint32_t items_cnt = (uint32_t)subinv.items.size();
      writer.write((char*)&items_cnt, 4);

      for (auto& entry : subinv.items)
      {
        auto item_node = entry.to_node();
        if (!item_node)
          return nullptr; // todo: don't leave this in this state

        // for now it is hacky, i use the first 15 bytes of itemData..
        auto& item_buf = item_node->data();
        writer.write(item_buf.data(), 15);
      
        writer.write_child(item_node);
      }
    }

    return writer.finalize("inventory");
  }
};

