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

//application to allow socket to be created at configuration time
//required to start/stop sending data during the simulation
class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

//application constructor
MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

/* static */
TypeId MyApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("MyApp")
    .SetParent<Application> ()
    .SetGroupName ("Tutorial")
    .AddConstructor<MyApp> ()
    ;
  return tid;
}

//initialize application member variables
void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

//called to start running application at appropriate time
void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

//stop creating simulation events
void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

//create and send packet
void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

//schedule next transmit event until application has sent enough
void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

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
  em->SetAttribute ("ErrorRate", DoubleValue (0.00001));
  p2pDevicesAB.Get(1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
  p2pDevicesCD.Get(1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
  //BER of B-C is 10^-5
  em->SetAttribute ("ErrorRate", DoubleValue (0.0001));
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
  sinkApps.Stop (Seconds (20.));

  //create socket on first node (A)
  Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (p2pNodes.Get (0), TcpSocketFactory::GetTypeId ());

  //start application on node A with data rate of 1Mbps
  Ptr<MyApp> app = CreateObject<MyApp> ();
  app->Setup (ns3TcpSocket, sinkAddress, 1040, 1000, DataRate ("1Mbps"));
  p2pNodes.Get (0)->AddApplication (app);
  app->SetStartTime (Seconds (1.));
  app->SetStopTime (Seconds (20.));

  //set up ASCII trace file for congestion window trace in tcpchain.cwnd
  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("tcpchain.cwnd");
  ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));

  //set up pcap file to trace packets
  PcapHelper pcapHelper;
  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile ("tcpchain.pcap", std::ios::out, PcapHelper::DLT_PPP);
  p2pDevicesCD.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, file));

  // make routing table 
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  //run simulation
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}

