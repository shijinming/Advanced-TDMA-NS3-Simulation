/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "hdv.h"
#include "ap-leader.h"
#include "ns3/wifi-net-device.h"
#include<stdlib.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("HumanApplication");
NS_OBJECT_ENSURE_REGISTERED(HumanApplication);

TypeId
HumanApplication::GetTypeId()
{
  static TypeId tid = TypeId("ns3::HumanApplication")
                          .SetParent<TDMAApplication>()
                          .AddConstructor<HumanApplication>();
  return tid;
}

HumanApplication::HumanApplication()
{
  NS_LOG_FUNCTION(this);
  m_status = Outter;
}

HumanApplication::~HumanApplication()
{
  NS_LOG_FUNCTION(this);
}

int HumanApplication::GetStatus()
{
  return m_status;
}

void HumanApplication::AddToMiddle()
{
  m_status = Middle;
  std::cout << GetNode()->GetId() << " add to middle." << Simulator::Now().GetMicroSeconds() << std::endl;
  if (isAtOwnSlot)
  {
    slotEndEvt.Cancel();
    txEvent.Cancel();
  }
  else
  {
    slotStartEvt.Cancel();
  }
  Time t1, t2, t3;
  t1 = Simulator::Now() - minTxInterval;
  t2 = (curSlot.CCHSlotNum + curSlot.SCHSlotNum) * slotSize;
  t3 = MilliSeconds(t1.GetMilliSeconds() % t2.GetMilliSeconds());
  if (t3.GetMilliSeconds() <= uint32_t(curSlot.apCCHSlotNum) * slotSize.GetMilliSeconds())
  {
    curSlot.start = curSlot.apCCHSlotNum * slotSize - t3;
  }
  else
    curSlot.start = t2 + curSlot.apCCHSlotNum * slotSize - t3;
  curSlot.duration = slotSize * curSlot.hdvCCHSlotNum - minTxInterval;
  slotStartEvt = Simulator::Schedule(curSlot.start, &HumanApplication::SlotStarted, this);
  isAtOwnSlot = false;
  // ChangeWindowSize (config.cwMin, config.cwMax);
  // ChangeWindowSize (config.cwMin, config.cwMax);
}

void HumanApplication::QuitFromMiddle()
{
  m_status = Outter;
  std::cout << GetNode()->GetId() << " quit from middle." << Simulator::Now().GetMicroSeconds() << std::endl;
  // ChangeWindowSize (15, 1023);
  // ChangeWindowSize (15, 1023);
}

void HumanApplication::ReceivePacket(Ptr<Packet> pkt, Address &srcAddr)
{
  InetSocketAddress inetAddr = InetSocketAddress::ConvertFrom(srcAddr);
  Ipv4Address addr = inetAddr.GetIpv4();
  // 获取发送该数据包的节点
  Ptr<Node> node = GetNodeFromAddress(addr);

  curSlot.id = Simulator::Now().GetMilliSeconds() / slotSize.GetMilliSeconds();

  if (IsAPApplicationInstalled(node))
  {
    // 收到了来自内核层的数据包
    if (m_status == Outter)
      AddToMiddle();
    receiveAPId = curSlot.id;
  }
  else if (curSlot.id - receiveAPId > 2 * (curSlot.CCHSlotNum + curSlot.SCHSlotNum)) //一个总帧内未收到内核层的包
  {
    if (m_status == Middle)
      QuitFromMiddle();
  }
}

Ptr<Node>
HumanApplication::GetNodeFromAddress(Ipv4Address &address)
{
  for (NodeList::Iterator n = NodeList::Begin();
       n != NodeList::End(); n++)
  {
    Ptr<Node> node = *n;
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    NS_ASSERT(ipv4);
    if (ipv4->GetInterfaceForAddress(address) != -1)
    {
      return node;
    }
  }
  return NULL;
}

bool HumanApplication::IsAPApplicationInstalled(Ptr<Node> node)
{
  uint32_t nApps = node->GetNApplications();
  for (uint32_t idx = 0; idx < nApps; idx++)
  {
    Ptr<Application> app = node->GetApplication(idx);
    if (dynamic_cast<APFollower *>(PeekPointer(app)) || dynamic_cast<APLeader *>(PeekPointer(app)))
    {
      return true;
    }
  }
  return false;
}

struct TDMASlot
HumanApplication::GetNextSlotInterval(void)
{
  //LOG_UNCOND ("Get Next Slot " << GetNode ()->GetId ());
  GetCurFrame();
  if (GetStatus() == Middle)
  {
    if (curSlot.curFrame == CCH_apFrame || curSlot.curFrame == SCH_hdvFrame)
    {
      curSlot.start = curSlot.apCCHSlotNum * slotSize + minTxInterval;
      curSlot.duration = slotSize * curSlot.hdvCCHSlotNum - minTxInterval;
    }
    else
    {
      curSlot.start = curSlot.apSCHSlotNum * slotSize + minTxInterval;
      curSlot.duration = slotSize * curSlot.hdvSCHSlotNum - minTxInterval;
    }
    Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (GetNode ()->GetDevice (0));
    Ptr<WifiPhy> phy = device->GetPhy ();
    phy->SetTxPowerStart(-1000);
    phy->SetTxPowerEnd(-1000);
  }
  else
  {
    curSlot.start = minTxInterval;
    curSlot.duration = Seconds(config.simTime);
  }
  return curSlot;
}

void HumanApplication::CreatePackets(uint32_t CpktCnt, uint32_t SpktCnt, uint32_t size)
{
  Ptr<Packet> pkt;
  for (uint32_t i = 0; i < CpktCnt; i++)
  {
    pkt = Create<Packet>(size);
    txqCCH.push(pkt);
  }
  for (uint32_t i = 0; i < SpktCnt; i++)
  {
    pkt = Create<Packet>(size);
    txqSCH.push(pkt);
  }
}

void HumanApplication::SendPacket(void)
{
  if (GetStatus() == Middle)
  {
    if (curSlot.curFrame == Frame::CCH_apFrame || curSlot.curFrame == Frame::CCH_hdvFrame)
      CreatePackets(7, 0, 1000);
    else
      CreatePackets(0, 7, 1000);
    EventId sendPacket;
    Time t = MicroSeconds(rand()%slotSize.GetMicroSeconds());
    sendPacket = Simulator::Schedule(t, &HumanApplication::SendOut, this);
  }
  else
  {
    GetCurFrame();
    if (curSlot.curFrame == Frame::CCH_apFrame || curSlot.curFrame == Frame::CCH_hdvFrame)
      CreatePackets(1, 0, 1000);
    else
      CreatePackets(0, 1, 1000);
    WakeUpTxQueue();
    EventId sendPacket;
    Time t = MilliSeconds(50);
    sendPacket = Simulator::Schedule(t, &HumanApplication::SendPacket, this);
  }
}

void HumanApplication::SlotWillStart(void)
{
  Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (GetNode ()->GetDevice (0));
  Ptr<WifiPhy> phy = device->GetPhy ();
  phy->SetTxPowerStart(config.txPower);
  phy->SetTxPowerEnd(config.txPower);
  SendPacket();
}
    
void HumanApplication::SendOut(void)
{
    WakeUpTxQueue();
}

void HumanApplication::ChangeWindowSize(uint32_t cwMin, uint32_t cwMax)
{
  Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice>(GetNode()->GetDevice(0));
  Ptr<OcbWifiMac> mac = DynamicCast<OcbWifiMac>(device->GetMac());
  // if not modified, cwmin = 15, cwmax = 1023
  mac->ConfigureEdca(cwMin, cwMax, 2, AC_BE_NQOS);
  mac->ConfigureEdca(cwMin, cwMax, 2, AC_VO);
  mac->ConfigureEdca(cwMin, cwMax, 3, AC_VI);
  mac->ConfigureEdca(cwMin, cwMax, 6, AC_BE);
  mac->ConfigureEdca(cwMin, cwMax, 9, AC_BK);
}

} // namespace ns3