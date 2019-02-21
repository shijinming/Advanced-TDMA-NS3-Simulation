/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef HDV_HELPER_H
#define HDV_HELPER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"

namespace ns3
{

class HumanApplicationHelper
{
public:
  HumanApplicationHelper ();

  Ptr<Application> Install (Ptr<Node> node) const;
  ApplicationContainer Install (NodeContainer c) const;
  void SetAttribute (std::string name, const AttributeValue (&value));
private:
  Ptr<Application> InstallPriv (Ptr<Node> node) const;
  ObjectFactory m_factory;
};

}


#endif