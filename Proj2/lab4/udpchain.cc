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
//   A----------B----------C----------D
//  
//     10.1.1.0   10.1.2.0   10.1.3.0 


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Lab4: udpchain");

int 
main (int argc, char *argv[])
{
  //add command line bool to ask whether or not ot log level info
  bool verbose = true;
  CommandLine cmd;
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.Parse (argc,argv);
  //if cmd line gives true then log level info
  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }
  
  //creat 4 p2p nodes
  NodeContainer p2pNodes;
  p2pNodes.Create (4);
  
  //set p2p attributes
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("10ms"));

  //set 3 p2p connections from A-B, B-C, C-D
  NetDeviceContainer p2pDevicesAB,p2pDevicesBC,p2pDevicesCD;
  p2pDevicesAB = pointToPoint.Install (p2pNodes.Get(0),p2pNodes.Get(1));
  p2pDevicesBC = pointToPoint.Install (p2pNodes.Get(1),p2pNodes.Get(2));
  p2pDevicesCD = pointToPoint.Install (p2pNodes.Get(2),p2pNodes.Get(3));

  //place nodes on the stack
  InternetStackHelper stack;
  stack.Install (p2pNodes);

  //assign different networks to each p2p connection
  Ipv4InterfaceContainer p2pInterfacesAB,p2pInterfacesBC,p2pInterfacesCD;
  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  p2pInterfacesAB = address.Assign (p2pDevicesAB);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  p2pInterfacesBC = address.Assign (p2pDevicesBC);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  p2pInterfacesCD = address.Assign (p2pDevicesCD);

  //echo server on port 9
  UdpEchoServerHelper echoServer (9);
 
  //install server on node D
  ApplicationContainer serverApps = echoServer.Install (p2pNodes.Get (3));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  //echo back to packet 
  UdpEchoClientHelper echoClient (p2pInterfacesCD.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (5));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  //set client as node A
  ApplicationContainer clientApps = echoClient.Install (p2pNodes.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  //make routing tables
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  
  //make pcap files
  pointToPoint.EnablePcapAll ("udpchain");
 
  //run simulation
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
