#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"

#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"

#include "ns3/point-to-point-module.h"
#include "ns3/net-device.h"

#include "ns3/traffic-control-module.h"

#include "ns3/config.h"
#include <vector>

#define DEBUG

#define CONFIG_START_TIME   2 
#define CONFIG_STOP_TIME    12

//#define CONFIG_NUMBER_OF_TERMINALS 300

#define CONFIG_INPUT_BUFFER_SIZE_BYTES 200000
//#define CONFIG_OUTPUT_BUFFER_SIZE_BYTES 200

// Sender data rate
#define CONFIG_SENDER_PACKET_SIZE 1448
//#define CONFIG_SENDER_INTERVAL_MEAN 10.0
#define CONFIG_SENDER_INTERVAL_BOUND 10.0
// Parameter 4, #define E

// sender link, data rate is bps; delay is in mili seconds;
#define CONFIG_SENDER_LINK_DATA_RATE "1Gbps"
#define CONFIG_SENDER_LINK_DELAY "10us"

// server link
//#define CONFIG_SERVER_LINK_DATA_RATE "1Gbps"
#define CONFIG_SERVER_LINK_DELAY "10ms"

#define	CONFIG_SENDER_PACKETS_PER_SHORT_FLOW 1000

using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE ("CsmaBridgeExample");

//static void
//CwndChange (uint32_t oldCwnd, uint32_t newCwnd)
//{
//  NS_LOG_UNCOND ( index << Simulator::Now ().GetSeconds () << "\t" << newCwnd);
//}

// This is the application defined in another class, this definition will allow us to hook the congestion window.
class MyApp : public Application 
{
public:

  MyApp ();
  virtual ~MyApp();

  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate, uint32_t numberPacketsPerFlow, double mean, double bound, double interval_threshold);

  Ptr<Socket>     m_socket;
private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;

  uint32_t 	  m_numberPacketsPerFlow;
  uint32_t        m_numberPacketsPerFlowCnt;

  double 	  m_threshold;
  Ptr<ExponentialRandomVariable> x;
};

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
	x = CreateObject<ExponentialRandomVariable> ();
        m_numberPacketsPerFlowCnt = 0;
}
static void
CwndChange (std::string context, uint32_t oldCwnd, uint32_t newCwnd)
{
  //NS_LOG_UNCOND (context << "\t" << Simulator::Now ().GetSeconds () << "\t" << newCwnd);
  
 // std::string tmp = context;
 // tmp.replace(tmp.find("/CongestionWindow"), string::npos, "");
 // cout << tmp << endl;
 // cout << "Matched objects: "<< Config::LookupMatches(tmp).GetN() << endl;
 // cout << Config::LookupMatches(tmp).Get(0)->GetInstanceTypeId() << endl;
  //cout << (ns3::Node)(Config::LookupMatches("/NodeList/0").Get(0)).GetApplication(0)->m_socket << endl;

  // Get all the nodes 
  NodeContainer global = NodeContainer::GetGlobal();
  // Get the node node index from the context
  int node_index = 0;
  Ptr<Application> ap = global.Get(node_index)->GetApplication(0);
  Ptr<MyApp> p = DynamicCast<MyApp>(ap);
  //cout << "Socket Address:\t " << p->m_socket << endl;

  cout << context << "\t"  << Simulator::Now ().GetNanoSeconds () << "\t" << "Socket Address:\t " << p->m_socket << "\t" << newCwnd << endl;
}

//static void
//RxDrop (Ptr<const Packet> p)
//{
//  NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
//}

// The following are queue related tracing functions.
// Packet drop event
static void 
AsciiDropEvent (std::string path, Ptr<const QueueItem> packet)
{
  //NS_LOG_UNCOND ("PacketDrop:\t" << Simulator::Now ().GetNanoSeconds () << "\t" << path << "\t" << *packet);
  cout << "PacketDrop:\t" << Simulator::Now ().GetNanoSeconds () << "\t" << path << "\t" << *packet << "\t" << *(packet->GetPacket())<< endl;
//  cout << "aaa" << endl;
//  *os << "d " << Simulator::Now ().GetSeconds () << " ";
//  *os << path << " " << *packet << std::endl;
}
// Enqueue event
static void 
AsciiEnqueueEvent (std::string path, Ptr<const QueueItem> packet)
{
  //NS_LOG_UNCOND ("Enqueue\t" << Simulator::Now ().GetNanoSeconds () << "\t" << *packet << *(packet->GetPacket()) );
  cout << "Enqueue\t" << Simulator::Now ().GetNanoSeconds () << "\t" << *(packet->GetPacket()) << endl;
 // *os << "+ " << Simulator::Now ().GetSeconds () << " ";
 // *os << path << " " << *packet << std::endl;
}

// Dequeue event
static void 
AsciiDequeueEvent (std::string path, Ptr<const QueueItem> packet)
{
  // NS_LOG_UNCOND ("Dequeue\t" << Simulator::Now ().GetNanoSeconds () << "\t" << *(packet->GetPacket()) );
  // NS_LOG_UNCOND ("Dequeue\t" << Simulator::Now ().GetNanoSeconds () );
  cout << "Dequeue\t" << Simulator::Now ().GetNanoSeconds () << endl;
 // *os << "+ " << Simulator::Now ().GetSeconds () << " ";
 // *os << path << " " << *packet << std::endl;
}

// Enqueue event net device
static void 
AsciiEnqueueEventNetDevice (std::string path, Ptr<const Packet> packet)
{
  // NS_LOG_UNCOND ("Dequeue\t" << Simulator::Now ().GetNanoSeconds () << "\t" << *(packet->GetPacket()) );
  // NS_LOG_UNCOND ("Dequeue\t" << Simulator::Now ().GetNanoSeconds () );
  cout << "EnqueueNetDevice\t" << Simulator::Now ().GetNanoSeconds () << "\t" << *packet << endl;
 // *os << "+ " << Simulator::Now ().GetSeconds () << " ";
 // *os << path << " " << *packet << std::endl;
}

// Dequeue event net device
static void 
AsciiDequeueEventNetDevice (std::string path, Ptr<const Packet> packet)
{
  // NS_LOG_UNCOND ("Dequeue\t" << Simulator::Now ().GetNanoSeconds () << "\t" << *(packet->GetPacket()) );
  // NS_LOG_UNCOND ("Dequeue\t" << Simulator::Now ().GetNanoSeconds () );
  cout << "DequeueNetDevice\t" << Simulator::Now ().GetNanoSeconds () << "\t" << *packet << endl;
 // *os << "+ " << Simulator::Now ().GetSeconds () << " ";
 // *os << path << " " << *packet << std::endl;
}

static void 
AsciiBytesInQueue (std::string path, uint32_t oldValue, uint32_t newValue) 
{
  //NS_LOG_UNCOND ("BytesInQueue\t" << Simulator::Now ().GetNanoSeconds () << "\t" << newValue);
  cout << "BytesInQueue\t" << Simulator::Now ().GetNanoSeconds () << "\t" << newValue << endl;
 // *os << "+ " << Simulator::Now ().GetSeconds () << " ";
 // *os << path << " " << *packet << std::endl;
}

static void 
AsciiPacketsInQueue (std::string path, uint32_t oldValue, uint32_t newValue) 
{
  //NS_LOG_UNCOND ("BytesInQueue\t" << Simulator::Now ().GetNanoSeconds () << "\t" << newValue);
  cout << "PacketsInQueue\t" << Simulator::Now ().GetNanoSeconds () << "\t" << newValue << endl;
 // *os << "+ " << Simulator::Now ().GetSeconds () << " ";
 // *os << path << " " << *packet << std::endl;
}

static void 
AsciiPacketsInQueueNetDevice (std::string path, uint32_t oldValue, uint32_t newValue) 
{
  //NS_LOG_UNCOND ("BytesInQueueNetDevice\t" << Simulator::Now ().GetNanoSeconds () << "\t" << newValue);
  cout << "BytesInQueueNetDevice\t" << Simulator::Now ().GetNanoSeconds () << "\t" << newValue << endl;
 // *os << "+ " << Simulator::Now ().GetSeconds () << " ";
 // *os << path << " " << *packet << std::endl;
}


MyApp::~MyApp()
{
  m_socket = 0;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate, uint32_t numberPacketsPerFlow, double mean, double bound, double threshold)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;

  x->SetAttribute ("Mean", DoubleValue (mean));
  //x->SetAttribute ("Bound", DoubleValue(bound));

  m_numberPacketsPerFlow = numberPacketsPerFlow;
  
  m_threshold = threshold;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

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

void 
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (m_running)
    {
      ScheduleTx ();
    }
}

void 
MyApp::ScheduleTx (void)
{

  // After a certain number of packets are sent, an interval is inserted.
  if (m_running)
    {
      Time tNext; 
      // The interval includes the transmission time of the packet;
      //tNext = Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ()) + x->GetValue());
      // The interval only includes the part which follows exponential distribution;
      tNext = Seconds (x->GetValue());
      // Terminate the connection if the interval is bigger than one value
     
      if (tNext.GetSeconds() > m_threshold)
      {
        cout << "Close the socket\t" << m_socket << "\t" << tNext.GetNanoSeconds() << endl;
        // Close the socket; 
      	m_socket = 0;  
	// Create a new socket;
     //	m_socket = Socket::CreateSocket (GetNode (), m_tid);
        m_socket = Socket::CreateSocket (this->GetNode(), TcpSocketFactory::GetTypeId ());
        m_socket->Bind ();
        m_socket->Connect (m_peer);

        cout << "Create the socket\t" << m_socket << "\t" << tNext.GetNanoSeconds() << endl;
        ostringstream oss;
       // oss << "/NodeList/" << this->GetNode() -> GetId ();

        oss << "/NodeList/" << this->GetNode()->GetId () << "/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow";
        m_socket->TraceConnect ("CongestionWindow", oss.str(), MakeCallback (&CwndChange));
      }
      cout << "/NodeList\t" << this->GetNode()->GetId() << "\t" << "Interval time\t" << tNext << endl;
      // Time is used to denote delay until the next event should execute.
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
   // cout << "couter " << m_numberPacketsPerFlowCnt << " " << m_running << endl;
   // cout << "start time " << m_startTime << " " << m_stopTime << endl;
}


string IpBaseGenerator (int index) {
	int second = 0, third = 0;
	second = (index / 255) + 1;
	third = index % 255;

	ostringstream oss;
	oss << "In Ip generator:" << "10." << second << "." << third << ".0";
	return oss.str();
}

int 
main (int argc, char *argv[])
{
  //
  // Users may find it convenient to turn on explicit debugging
  // for selected modules; the below lines suggest how to do this
  //
#if 0 
  LogComponentEnable ("CsmaBridgeExample", LOG_LEVEL_INFO);
#endif

  //
  // Allow the user to override any of the defaults and the above Bind() at
  // run-time, via command-line arguments
  //

  int 			CONFIG_NUMBER_OF_TERMINALS 			=	1;
  double 		CONFIG_SENDER_INTERVAL_MEAN 			=	0.002;
  double		CONFIG_SENDER_INTERVAL_THRESHOLD		=	0.004;
  unsigned long long 	CONFIG_OUTPUT_BUFFER_SIZE_BYTES 		=	100000;
  unsigned long long 	CONFIG_SERVER_LINK_DATA_RATE 			=	1000000;
  cout << argc << endl;
  //int 			CONFIG_NUMBER_OF_TERMINALS 			=	atol(argv[1]);
  //double 		CONFIG_SENDER_INTERVAL_MEAN 			=	atof(argv[2]);
  //double		CONFIG_SENDER_INTERVAL_THRESHOLD		=	atof(argv[3]);
  //unsigned long long 	CONFIG_OUTPUT_BUFFER_SIZE_BYTES 		=	atol(argv[4]);
  //unsigned long long 	CONFIG_SERVER_LINK_DATA_RATE 			=	atol(argv[5]);

  CommandLine cmd;
  cmd.AddValue ("CONFIG_NUMBER_OF_TERMINALS", "The number of the senders", CONFIG_NUMBER_OF_TERMINALS 		);
  cmd.AddValue ("CONFIG_SENDER_INTERVAL_MEAN", "The mean of the packet interval", CONFIG_SENDER_INTERVAL_MEAN 		);
  cmd.AddValue ("CONFIG_SERVER_LINK_DATA_RATE", "The link capacity between the router and the server", CONFIG_SERVER_LINK_DATA_RATE 		);
  cmd.AddValue ("CONFIG_OUTPUT_BUFFER_SIZE_BYTES", "The buffer size", CONFIG_OUTPUT_BUFFER_SIZE_BYTES 	);
  cmd.Parse (argc, argv);

  NS_LOG_UNCOND (CONFIG_NUMBER_OF_TERMINALS << " " << CONFIG_SENDER_INTERVAL_MEAN << " " << CONFIG_SENDER_INTERVAL_THRESHOLD << " "  << CONFIG_OUTPUT_BUFFER_SIZE_BYTES << " " << CONFIG_SERVER_LINK_DATA_RATE);

  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
  Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1310720));

  // Create the nodes;
  int numberOfTerminals = CONFIG_NUMBER_OF_TERMINALS;
  NS_LOG_INFO ("Create sender nodes.");
  NodeContainer terminals;
  terminals.Create (numberOfTerminals);

  NS_LOG_INFO ("Create server node.");
  NodeContainer servers;
  servers.Create (1);

  NodeContainer router;
  router.Create (1);

  // Add internet stack to the terminals;
  InternetStackHelper internet;
  internet.Install (terminals);
  internet.Install (servers);
  internet.Install (router);

  NS_LOG_INFO ("Build Topology");
  NS_LOG_INFO ("Create the wifi devices on terminals and the accessing point");

  NodeContainer wifiApNode = router.Get (0);
  //NodeContainer & wifiStaNodes = terminals; 

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  //staDevices = wifi.Install (phy, mac, wifiStaNodes);
  staDevices = wifi.Install (phy, mac, terminals);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
 // mobility.Install (wifiStaNodes);
  mobility.Install (terminals);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);

  // The server link
  PointToPointHelper p2p;
  p2p.SetQueue("ns3::DropTailQueue", "MaxBytes", UintegerValue(CONFIG_INPUT_BUFFER_SIZE_BYTES), "Mode", EnumValue (DropTailQueue::QUEUE_MODE_BYTES));
  p2p.SetDeviceAttribute ("DataRate", DataRateValue (CONFIG_SERVER_LINK_DATA_RATE));
  p2p.SetChannelAttribute ("Delay", StringValue (CONFIG_SERVER_LINK_DELAY));

  // Create point to point link, from the server to the bridge
  NetDeviceContainer linkServer = p2p.Install (NodeContainer (servers.Get (0), router));

  // Set the size of the TC layer queue
  TrafficControlHelper tch;
  tch.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "Limit", UintegerValue(10));
  tch.Install(linkServer.Get(1));

  Ipv4AddressHelper address;
  Ipv4InterfaceContainer serverIpv4; 
  address.SetBase ("10.1.64.0", "255.255.192.0");
  Ipv4InterfaceContainer p2pInterfaces;
  serverIpv4 = address.Assign (linkServer);

  address.SetBase ("10.2.128.0", "255.255.192.0");
  address.Assign (staDevices);
  address.Assign (apDevices);

  // Create router nodes.
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Add the trace callback function.
  ostringstream oss;
  oss << "/NodeList/" << router.Get (0) -> GetId () << "/$ns3::TrafficControlLayer/RootQueueDiscList/" << linkServer.Get (1)->GetIfIndex() << "/Enqueue";
  cout << oss.str() << endl;
  Config::Connect (oss.str(), MakeCallback (&AsciiEnqueueEvent));

  oss.str("");
  oss.clear();
  oss << "/NodeList/" << router.Get (0) -> GetId () << "/$ns3::TrafficControlLayer/RootQueueDiscList/" << linkServer.Get (1)->GetIfIndex() << "/Dequeue";
  cout << oss.str() << endl;
  Config::Connect (oss.str(), MakeCallback (&AsciiDequeueEvent));

  oss.str("");
  oss.clear();
  oss << "/NodeList/" << router.Get (0) -> GetId () << "/$ns3::TrafficControlLayer/RootQueueDiscList/" << linkServer.Get (1)->GetIfIndex() << "/Drop";
  cout << oss.str() << endl;
  Config::Connect (oss.str(), MakeCallback (&AsciiDropEvent));

  oss.str("");
  oss.clear();
  oss << "/NodeList/" << router.Get (0) -> GetId () << "/$ns3::TrafficControlLayer/RootQueueDiscList/" << linkServer.Get (1)->GetIfIndex() << "/BytesInQueue";
  cout << oss.str() << endl;
  Config::Connect (oss.str(), MakeCallback (&AsciiBytesInQueue));

  oss.str("");
  oss.clear();
  oss << "/NodeList/" << router.Get (0) -> GetId () << "/$ns3::TrafficControlLayer/RootQueueDiscList/" << linkServer.Get (1)->GetIfIndex() << "/PacketsInQueue";
  cout << oss.str() << endl;
  Config::Connect (oss.str(), MakeCallback (&AsciiPacketsInQueue));

  // This is to log down the number of bytes in the queue on the net device.
  oss.str("");
  oss.clear();
  oss << "/NodeList/" << router.Get (0) -> GetId () << "/DeviceList/" << linkServer.Get (1)->GetIfIndex() << "/$ns3::PointToPointNetDevice/TxQueue/BytesInQueue";
  //oss << "/NodeList/" << router.Get (0) -> GetId () << "/$ns3::TrafficControlLayer/RootQueueDiscList/" << linkServer.Get (1)->GetIfIndex() << "/BytesInQueue";
  cout << oss.str() << endl;
  Config::Connect (oss.str(), MakeCallback (&AsciiPacketsInQueueNetDevice));

  // This is to log down the dequeue event of the net device.
  oss.str("");
  oss.clear();
  oss << "/NodeList/" << router.Get (0) -> GetId () << "/DeviceList/" << linkServer.Get (1)->GetIfIndex() << "/$ns3::PointToPointNetDevice/TxQueue/Enqueue";
  //oss << "/NodeList/" << router.Get (0) -> GetId () << "/$ns3::TrafficControlLayer/RootQueueDiscList/" << linkServer.Get (1)->GetIfIndex() << "/BytesInQueue";
  cout << oss.str() << endl;
  Config::Connect (oss.str(), MakeCallback (&AsciiEnqueueEventNetDevice));

  // This is to log down the dequeue event of the net device.
  oss.str("");
  oss.clear();
  oss << "/NodeList/" << router.Get (0) -> GetId () << "/DeviceList/" << linkServer.Get (1)->GetIfIndex() << "/$ns3::PointToPointNetDevice/TxQueue/Dequeue";
  //oss << "/NodeList/" << router.Get (0) -> GetId () << "/$ns3::TrafficControlLayer/RootQueueDiscList/" << linkServer.Get (1)->GetIfIndex() << "/BytesInQueue";
  cout << oss.str() << endl;
  Config::Connect (oss.str(), MakeCallback (&AsciiDequeueEventNetDevice));

   // Create a sink application on the server node to receive these applications. 
   uint16_t port = 50000;
   Address sinkLocalAddress (InetSocketAddress (serverIpv4.GetAddress(0), port));
   PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
   ApplicationContainer sinkApp = sinkHelper.Install (servers.Get (0));
   sinkApp.Start (Seconds (CONFIG_START_TIME - 1));
   sinkApp.Stop (Seconds (CONFIG_STOP_TIME));

   // Create all the sockets.
   vector<Ptr<Socket> > SocketVector(numberOfTerminals);
   for (vector<Ptr<Socket> >::iterator it = SocketVector.begin(); it < SocketVector.end(); it++) {
     int nodeIndex = it - SocketVector.begin();
     *it = Socket::CreateSocket (terminals.Get (nodeIndex), TcpSocketFactory::GetTypeId ());
     ostringstream oss;
     oss << "/NodeList/" << terminals.Get (nodeIndex)->GetId () << "/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow";
     //cout << oss.str() << endl;
     (*it)->TraceConnect ("CongestionWindow", oss.str(), MakeCallback (&CwndChange));
   }

   ApplicationContainer clientApps;
   vector<Ptr<MyApp> > ApplicationVector(numberOfTerminals);
   for(uint32_t i=0; i<terminals.GetN (); ++i)
   {
      Address sinkAddress (InetSocketAddress (serverIpv4.GetAddress (0), port)); 

      ApplicationVector[i] = CreateObject<MyApp> ();
      // void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);
      // number of packets is not used here.

      // construct a string to denote the rate
      ApplicationVector[i]->Setup (SocketVector[i], sinkAddress, CONFIG_SENDER_PACKET_SIZE, 1000, DataRate (string (CONFIG_SENDER_LINK_DATA_RATE)), CONFIG_SENDER_PACKETS_PER_SHORT_FLOW, CONFIG_SENDER_INTERVAL_MEAN, CONFIG_SENDER_INTERVAL_BOUND, CONFIG_SENDER_INTERVAL_THRESHOLD);
      terminals.Get (i)->AddApplication (ApplicationVector[i]);
      clientApps.Add (ApplicationVector[i]);
   }
   clientApps.Start (Seconds (CONFIG_START_TIME));
   clientApps.Stop (Seconds (CONFIG_STOP_TIME));


  NS_LOG_INFO ("Configure Tracing.");

  //
  // Configure tracing of all enqueue, dequeue, and NetDevice receive events.
  // Trace output will be sent to the file "csma-bridge.tr"
  //
  AsciiTraceHelper ascii;
  p2p.EnableAsciiAll (ascii.CreateFileStream ("csma-bridge.tr"));

  //
  // Also configure some tcpdump traces; each interface will be traced.
  // The output files will be named:
  //     csma-bridge-<nodeId>-<interfaceId>.pcap
  // and can be read by the "tcpdump -r" command (use "-tt" option to
  // display timestamps correctly)
  //
  //p2p.EnablePcapAll ("csma-bridge", false);
  Simulator::Stop (Seconds (CONFIG_STOP_TIME));
  //
  // Now, do the actual simulation.
  //
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}


