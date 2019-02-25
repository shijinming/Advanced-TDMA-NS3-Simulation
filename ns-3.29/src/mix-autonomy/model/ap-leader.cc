/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ap-leader.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("APLeader");
NS_OBJECT_ENSURE_REGISTERED (APLeader);

TypeId
APLeader::GetTypeId ()
{
    static TypeId tid = TypeId ("ns3::APLeader")
        .SetParent <APFollower> ()
        .AddConstructor <APLeader> ()
    ;
    return tid;
}

APLeader::APLeader ()
{
  
}

APLeader::~APLeader ()
{
  
}

void
APLeader::ReceivePacketFromAP (Ptr<Packet> pkt)
{
  PacketHeader pHeader;
  pkt->RemoveHeader(pHeader);
  if(pHeader.GetIsLeader())
    return;
  std::map <uint16_t, uint32_t>::iterator iter;
  iter = m_queueLen.find(pHeader.GetId());
  if(iter != m_queueLen.end())
    {
      m_queueLen[iter->first] = pHeader.GetQueueLen();
    }
  else
  {
    m_queueLen.insert(std::pair<uint16_t, uint16_t> (pHeader.GetId(), pHeader.GetQueueLen()));
  }
}

void
APLeader::SetupHeader(PacketHeader &hdr)
{
  APFollower::SetupHeader(hdr);
  hdr.SetIsLeader(true);
  hdr.SetCCHslotAllocation(m_CCHslotAllocation);
  hdr.SetSCHslotAllocation(m_SCHslotAllocation);
}

bool
APLeader::SlotAllocation ()
{
  std::map <uint16_t, uint32_t>::iterator iter;
  iter = m_queueLen.find(GetNode () -> GetId());
  if(iter != m_queueLen.end())
    {
      m_queueLen[iter->first] = txq.size();
    }
  else
  {
    m_queueLen.insert(std::pair<uint16_t, uint16_t> (GetNode ()->GetId(), txq.size ()));
  }
  int totalLen = 0;
  for(iter = m_queueLen.begin(); iter != m_queueLen.end(); iter++)
  {
    totalLen+=iter->second;
  }
  m_CCHslotAllocation.clear();
  m_SCHslotAllocation.clear();
  for(iter = m_queueLen.begin(); iter != m_queueLen.end(); iter++)
  {
    m_CCHslotAllocation.push_back(iter->first);
    if(totalLen > 0)
    {
      for(uint32_t i = 0; i < iter->second * curSlot.SCHSlotNum / totalLen; i++){
        m_SCHslotAllocation.push_back(iter->first);
      }
    } 
  }

  //查找leader给自己分配的数据帧发包时隙
  std::vector <uint64_t> mySendSlot;
  for(uint32_t i=0; i<curSlot.SCHSlotNum; i++) //查找发数据包的时隙
  {
    if(GetNode ()->GetId () == m_SCHslotAllocation[i])
      {
        mySendSlot.push_back(i); //将i插入到向量最后面
      } 
  }

  //判断此时是否为leader发包的时隙
  if(curSlot.frameId == curSlot.apCCHSlotNum - 1 && curSlot.curFrame == CCH_apFrame)
     {
       return true;
     } 
  if(curSlot.curFrame == SCH_apFrame)
    {
      for (uint64_t i = 0; i < mySendSlot.size(); i++)
      {
        if(curSlot.frameId == mySendSlot[i]) return true;
      }
    }
  return false;
}

}
