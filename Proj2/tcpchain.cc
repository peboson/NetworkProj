/*
author: Nathan Turner, Jason Song
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
#include "ns3/applications-module.h"
#include <fstream>
#include <string>
#include "ns3/packet-sink.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Lab5: tcpchain");


// Default Network Topology
//
//         node A                node B                node C                node D
//   +----------------+    +----------------+    +----------------+    +----------------+
//   |    ns-3 TCP    |    |    ns-3 TCP    |    |    ns-3 TCP    |    |    ns-3 TCP    |
//   +----------------+    +----------------+    +----------------+    +----------------+
//   |    10.1.1.1    |    |    10.1.2.1    |    |    10.1.3.1    |    |    10.1.3.2    |
//   +----------------+    +----------------+    +----------------+    +----------------+
//   | point-to-point |    | point-to-point |    | point-to-point |    | point-to-point |
//   +----------------+    +----------------+    +----------------+    +----------------+
//           |                     |                     |                     |
//           +---------------------+---------------------+---------------------+
//                5 Mbps, 2 ms          5 Mbps, 2 ms          5 Mbps, 2 ms
//
//


//trace callbacks from TCP indicating congestion window has been updated
//place in log and file stream
static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
}

//show where packets are dropped to log and file stream
static void
RxDrop (Ptr<PcapFileWrapper> file, Ptr<const Packet> p)
{
  NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
  file->Write (Simulator::Now (), p);
}
std::ofstream testFile("test.txt");
static int RecvedPackages;

//show where packets are received to log and file stream
static void
RxEnd (Ptr<PcapFileWrapper> file, Ptr<const Packet> p)
{
  RecvedPackages++;
  NS_LOG_UNCOND ("RxEnd at " << Simulator::Now ().GetSeconds ()<<" Uid: "<<p->GetUid()<<" Accumulate Rx pack: "<<RecvedPackages);
  //file->Write (Simulator::Now (), p);
  p->PrintPacketTags(testFile);
}

static int TxPackages;

//show where packets are received to log and file stream
static void
TxEnd (Ptr<PcapFileWrapper> file, Ptr<const Packet> p)
{
  TxPackages++;
  NS_LOG_UNCOND ("TxEnd at " << Simulator::Now ().GetSeconds ()<<" Uid: "<<p->GetUid()<<" Accumulate Tx pack: "<<TxPackages);
  //file->Write (Simulator::Now (), p);
  p->PrintPacketTags(testFile);
}

int
main (int argc, char *argv[])
{
  //add command line bool to ask whether or not ot log level info, and BER of inner connection BC with output cwnd file name
  bool verbose = true;
  double ber;
  std::string cwndFile;
  CommandLine cmd;
  cmd.AddValue ("errorRate", "Tell echo applications to log if true", ber);
  cmd.AddValue ("outputCwnd", "Tell echo applications to log if true", cwndFile);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.Parse (argc,argv);
  //if cmd line gives true then log level info
  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

  //creat 4 p2p nodes
  uint32_t nP2pNodes = 4;
  NodeContainer p2pNodes;
  p2pNodes.Create (nP2pNodes);

  //set p2p attributes
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  //set 3 p2p connections from A-B, B-C, C-D
  NetDeviceContainer p2pDevicesAB,p2pDevicesBC,p2pDevicesCD;
  p2pDevicesAB = pointToPoint.Install (p2pNodes.Get(0),p2pNodes.Get(1));
  p2pDevicesBC = pointToPoint.Install (p2pNodes.Get(1),p2pNodes.Get(2));
  p2pDevicesCD = pointToPoint.Install (p2pNodes.Get(2),p2pNodes.Get(3));

  //introduce error into channel at a given rate
  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  //BER of A-B and C-D is 10^-6
  em->SetAttribute ("ErrorRate", DoubleValue (0.000001));
  p2pDevicesAB.Get(1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
  p2pDevicesCD.Get(1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
  //BER of B-C is 10^-5 or 5 times worse (value input by user
  em->SetAttribute ("ErrorRate", DoubleValue (ber));
  p2pDevicesBC.Get(1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

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

  //sink used to receive connections and data on node D
  uint16_t sinkPort = 8080;
  Address sinkAddress (InetSocketAddress (p2pInterfacesCD.GetAddress (1), sinkPort));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  ApplicationContainer sinkApps = packetSinkHelper.Install (p2pNodes.Get (nP2pNodes-1));
  sinkApps.Start (Seconds (0.));
  sinkApps.Stop (Seconds (200.));

  //create socket on first node (A)
  Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (p2pNodes.Get (0), TcpSocketFactory::GetTypeId ());
/*
  //start application on node A with data rate of 1Mbps
  Ptr<MyApp> app = CreateObject<MyApp> ();
  app->Setup (ns3TcpSocket, sinkAddress, 1040, 1000, DataRate (".1Mbps"));
  p2pNodes.Get (0)->AddApplication (app);
  app->SetStartTime (Seconds (1.));
  app->SetStopTime (Seconds (200.));
*/

  int maxBytes=10000;
  BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (p2pInterfacesCD.GetAddress (1), sinkPort));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  ApplicationContainer sourceApps = source.Install (p2pNodes.Get (0));
  sourceApps.Start (Seconds (0.0));
  sourceApps.Stop (Seconds (200.0));


  //set up ASCII trace file for congestion window trace in tcpchain.cwnd
  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (cwndFile);
  ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));

  //set up pcap file to trace packets
  PcapHelper pcapHelper;
  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile ("tcpchain.pcap", std::ios::out, PcapHelper::DLT_PPP);
  p2pDevicesCD.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, file));

  RecvedPackages=0;
  TxPackages=0;
  //trace recieved packets
  p2pDevicesCD.Get (1)->TraceConnectWithoutContext ("PhyRxEnd", MakeBoundCallback (&RxEnd, file));
  p2pDevicesCD.Get (1)->TraceConnectWithoutContext ("PhyTxEnd", MakeBoundCallback (&TxEnd, file));
  
  // make routing table 
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  pointToPoint.EnablePcapAll ("tcpchain");

  //run simulation
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}

