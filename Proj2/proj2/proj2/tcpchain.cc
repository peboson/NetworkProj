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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Proj2");

//number of bytes to send each simulation
static const uint32_t totalTxBytes = 1000000;
static uint32_t currentTxBytes = 0;
//perform series of 1000 byte writes
static const uint32_t writeSize = 1000;
uint8_t data[writeSize];

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

void StartFlow (Ptr<Socket>, Ipv4Address, uint16_t);
void WriteUntilBufferFull (Ptr<Socket>, uint32_t);

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


int
main (int argc, char *argv[])
{
  //add command line bool to ask whether or not ot log level info, and BER of inner connection BC with output cwnd file name
  bool verbose = true;
  double ber = .00001;
  std::string cwndFile = "tcp.cwnd";
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
  uint16_t serverPort = 8080;
  PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), serverPort));
  ApplicationContainer apps = sink.Install (p2pNodes.Get (nP2pNodes-1));
  apps.Start (Seconds (0.));
  apps.Stop (Seconds (200.));

  //create and bind socket on first node (A)
  Ptr<Socket> localSocket = Socket::CreateSocket (p2pNodes.Get (0), TcpSocketFactory::GetTypeId ());
  localSocket->Bind();

  //trace congestion window in tcpchain.cwnd
  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (cwndFile);
  localSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));

  //set up pcap file to trace packets
  PcapHelper pcapHelper;
  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile ("tcpchain.pcap", std::ios::out, PcapHelper::DLT_PPP);
  p2pDevicesCD.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, file));

  //schedule sending application
  Simulator::ScheduleNow(&StartFlow, localSocket, p2pInterfacesCD.GetAddress (1), serverPort);

  // make routing table 
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  pointToPoint.EnablePcapAll ("tcpchain");

  //run simulation
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}

//begin implementation of sending "Application"
void StartFlow (Ptr<Socket> localSocket,
                Ipv4Address servAddress,
                uint16_t servPort)
{
  NS_LOG_LOGIC ("Starting flow at time " <<  Simulator::Now ().GetSeconds ());
  localSocket->Connect (InetSocketAddress (servAddress, servPort)); //connect

  // tell the tcp implementation to call WriteUntilBufferFull again
  // if we blocked and new tx buffer space becomes available
  localSocket->SetSendCallback (MakeCallback (&WriteUntilBufferFull));
  WriteUntilBufferFull (localSocket, localSocket->GetTxAvailable ());
}

void WriteUntilBufferFull (Ptr<Socket> localSocket, uint32_t txSpace)
{
  while (currentTxBytes < totalTxBytes && localSocket->GetTxAvailable () > 0)
    {
      uint32_t left = totalTxBytes - currentTxBytes;
      uint32_t dataOffset = currentTxBytes % writeSize;
      uint32_t toWrite = writeSize - dataOffset;
      toWrite = std::min (toWrite, left);
      toWrite = std::min (toWrite, localSocket->GetTxAvailable ());
      int amountSent = localSocket->Send (&data[dataOffset], toWrite, 0);
      if(amountSent < 0)
        {
          // we will be called again when new tx space becomes available.
          return;
        }
      currentTxBytes += amountSent;
    }
  localSocket->Close ();
}

