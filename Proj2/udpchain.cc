/*
author: Nathan Turner
date:3/27/16
ussage: ./waf --run scratch/udpchain.cc
*/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

// Default Network Topology
//
//   A    B    C    D
//   |    |    |    |
//   ================
//     LAN 10.1.1.0


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Lab4: udpchain");

int 
main (int argc, char *argv[])
{
  bool verbose = true;

  CommandLine cmd;
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

  cmd.Parse (argc,argv);

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
export NS_LOG=UdpEchoClientApplication=level_all;
    }

  NodeContainer csmaNodes;
  csmaNodes.Create (4);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("1Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (10)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  InternetStackHelper stack;
  stack.Install (csmaNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (3));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (3), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (5));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (csmaNodes.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

//  pointToPoint.EnablePcapAll ("second");
  csma.EnablePcap ("udpchain", csmaDevices.Get (0), true);
  csma.EnablePcap ("udpchain", csmaDevices.Get (1), true);
  csma.EnablePcap ("udpchain", csmaDevices.Get (2), true);
  csma.EnablePcap ("udpchain", csmaDevices.Get (3), true);

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
