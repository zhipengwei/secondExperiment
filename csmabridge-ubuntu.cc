#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"

#include "ns3/point-to-point-module.h"
#include "ns3/net-device.h"

#include "ns3/traffic-control-module.h"

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

static void
CwndChange (std::string context, uint32_t oldCwnd, uint32_t newCwnd)
{
  //NS_LOG_UNCOND (context << "\t" << Simulator::Now ().GetSeconds () << "\t" << newCwnd);
  cout << context << "\t" << Simulator::Now ().GetSeconds () << "\t" << newCwnd << endl;
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
  cout << "PacketDrop:\t" << Simulator::Now ().GetNanoSeconds () << "\t" << path << "\t" << *packet << endl;
//  cout << "aaa" << endl;
//  *os << "d " << Simulator::Now ().GetSeconds () << " ";
//  *os << path << " " << *packet << std::endl;
}
// Enqueue event
static void 
AsciiEnqueueEvent (std::string path, Ptr<const QueueItem> packet)
{
  //NS_LOG_UNCOND ("Enqueue\t" << Simulator::Now ().GetNanoSeconds () << "\t" << *packet << *(packet->GetPacket()) );
  cout << "Enqueue\t" << Simulator::Now ().GetNanoSeconds () << endl;
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

// Dequeue event net device
static void 
AsciiDequeueEventNetDevice (std::string path, Ptr<const Packet> packet)
{
  // NS_LOG_UNCOND ("Dequeue\t" << Simulator::Now ().GetNanoSeconds () << "\t" << *(packet->GetPacket()) );
  // NS_LOG_UNCOND ("Dequeue\t" << Simulator::Now ().GetNanoSeconds () );
  cout << "Dequeue\t" << Simulator::Now ().GetNanoSeconds () << endl;
 // *os << "+ " << Simulator::Now ().GetSeconds () << " ";
 // *os << path << " " << *packet << std::endl;
}

static void 
AsciiPacketsInQueue (std::string path, uint32_t oldValue, uint32_t newValue) 
{
  //NS_LOG_UNCOND ("BytesInQueue\t" << Simulator::Now ().GetNanoSeconds () << "\t" << newValue);
  cout << "BytesInQueue\t" << Simulator::Now ().GetNanoSeconds () << "\t" << newValue << endl;
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

// This is the application defined in another class, this definition will allow us to hook the congestion window.
class MyApp : public Application 
{
public:

  MyApp ();
  virtual ~MyApp();

  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate, uint32_t numberPacketsPerFlow, double mean, double bound, double interval_threshold);

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
     
      if (tNext > m_threshold)
      {
        // Close the socket; 
      	m_socket = 0;  
	// Create a new socket;
     	m_socket = Socket::CreateSocket (GetNode (), m_tid);
        m_socket->Bind ();
        m_socket->Connect (m_peer);

        ostringstream oss;
        oss << "/NodeList/" << this->GetNode() -> GetId ();

        m_socket->TraceConnect ("CongestionWindow", oss.str(), MakeCallback (&CwndChange));
      }
      cout << "/NodeList\t" << this->GetNode() << "\t" << "Interval time\t" << tNext << endl;
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

 // int 		CONFIG_NUMBER_OF_TERMINALS 			=	500;
 // double 		CONFIG_SENDER_INTERVAL_MEAN 			=	0.0016;
 // unsigned long long 	CONFIG_SERVER_LINK_DATA_RATE 			=	1000000000;
 // unsigned long long 	CONFIG_OUTPUT_BUFFER_SIZE_BYTES 		=	5242880;
  int 			CONFIG_NUMBER_OF_TERMINALS 			=	atol(argv[1]);
  double 		CONFIG_SENDER_INTERVAL_MEAN 			=	atof(argv[2]);
  double		CONFIG_SENDER_INTERVAL_THRESHOLD		=	atof(argv[3]);
  unsigned long long 	CONFIG_OUTPUT_BUFFER_SIZE_BYTES 		=	atol(argv[4]);
  unsigned long long 	CONFIG_SERVER_LINK_DATA_RATE 			=	atol(argv[5]);

  CommandLine cmd;
  cmd.AddValue ("CONFIG_NUMBER_OF_TERMINALS", "The number of the senders", CONFIG_NUMBER_OF_TERMINALS 		);
  cmd.AddValue ("CONFIG_SENDER_INTERVAL_MEAN", "The mean of the packet interval", CONFIG_SENDER_INTERVAL_MEAN 		);
  cmd.AddValue ("CONFIG_SERVER_LINK_DATA_RATE", "The link capacity between the router and the server", CONFIG_SERVER_LINK_DATA_RATE 		);
  cmd.AddValue ("CONFIG_OUTPUT_BUFFER_SIZE_BYTES", "The buffer size", CONFIG_OUTPUT_BUFFER_SIZE_BYTES 	);
  cmd.Parse (argc, argv);

  //
  // Explicitly create the nodes required by the topology (shown above).
  NS_LOG_UNCOND (CONFIG_NUMBER_OF_TERMINALS << " " << CONFIG_SENDER_INTERVAL_MEAN << " " << CONFIG_SERVER_LINK_DATA_RATE << " " << CONFIG_OUTPUT_BUFFER_SIZE_BYTES);

  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
  Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1310720));

  int numberOfTerminals = CONFIG_NUMBER_OF_TERMINALS;
  NS_LOG_INFO ("Create sender nodes.");
  NodeContainer terminals;
  terminals.Create (numberOfTerminals);

  NS_LOG_INFO ("Create server node.");
  NodeContainer servers;
  servers.Create (1);

  NodeContainer router;
  router.Create (1);

  // Add internet stack to the terminals
  InternetStackHelper internet;
  internet.Install (terminals);
  internet.Install (servers);
  internet.Install (router);

  NS_LOG_INFO ("Build Topology");
  // The terminal link
  PointToPointHelper p2p;

  p2p.SetDeviceAttribute ("DataRate", StringValue (CONFIG_SENDER_LINK_DATA_RATE));
  p2p.SetChannelAttribute ("Delay", StringValue (CONFIG_SENDER_LINK_DELAY));
  p2p.SetQueue("ns3::DropTailQueue", "MaxBytes", UintegerValue(CONFIG_INPUT_BUFFER_SIZE_BYTES), "Mode", EnumValue (DropTailQueue::QUEUE_MODE_BYTES));

  // Create the point to point links from each terminal to the router
  NetDeviceContainer terminalDevices;
  NetDeviceContainer routerDevices;

  vector<Ipv4InterfaceContainer> TerminalIpv4Interface;
  Ipv4AddressHelper ipv4;
  for (int i = 0; i < numberOfTerminals; i++) {
    NetDeviceContainer link = p2p.Install (NodeContainer (terminals.Get (i), router));
    terminalDevices.Add (link.Get (0));
    routerDevices.Add (link.Get (1));
    cout << "Assign ip address to node:" << i << endl;  
    string ip_string = IpBaseGenerator (i+1); 
    char ip_char[25];
    for (unsigned int j = 0; j < ip_string.size(); j++)
        ip_char[j] = ip_string.at(j);
    ip_char[ip_string.size()] = '\0';
    ipv4.SetBase (ip_char, Ipv4Mask("255.255.255.0"));
    TerminalIpv4Interface.push_back (ipv4.Assign (link));
  }

  // The server link
  // UintegerValue, holds an unsigned integer type.
  //p2p.SetQueue("ns3::DropTailQueue", "MaxBytes", UintegerValue(CONFIG_OUTPUT_BUFFER_SIZE_BYTES), "Mode", EnumValue (DropTailQueue::QUEUE_MODE_BYTES));
  p2p.SetQueue("ns3::DropTailQueue", "MaxBytes", UintegerValue (15000), "Mode", EnumValue (DropTailQueue::QUEUE_MODE_BYTES));
  p2p.SetDeviceAttribute ("DataRate", DataRateValue (CONFIG_SERVER_LINK_DATA_RATE));
  p2p.SetChannelAttribute ("Delay", StringValue (CONFIG_SERVER_LINK_DELAY));

  // Create point to point link, from the server to the bridge
  NetDeviceContainer serverDevices;
  NetDeviceContainer linkServer = p2p.Install (NodeContainer (servers.Get (0), router));

  // Set the size of the TC layer queue
  TrafficControlHelper tch;
  //uint16_t handle = tch.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "Limit", UintegerValue(100));
  tch.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "Limit", UintegerValue(CONFIG_OUTPUT_BUFFER_SIZE_BYTES/1500 - 10));
  // tch.AddInternalQueues (handle, 3, "ns3::DropTailQueue", "MaxPackets", UintegerValue (CONFIG_OUTPUT_BUFFER_SIZE_BYTES/1500 - 10));
  //tch.AddInternalQueues (handle, 3, "ns3::DropTailQueue", "MaxPackets", UintegerValue (100));
  tch.Install(linkServer.Get(1));

  serverDevices.Add (linkServer.Get (0));
  routerDevices.Add (linkServer.Get (1));

  // We've got the "hardware" in place.  Now we need to add IP addresses.
  NS_LOG_INFO ("Assign IP Addresses.");
  string ip_string = IpBaseGenerator (numberOfTerminals + 1); 
  char ip_char[20];
  for (unsigned int j = 0; j < ip_string.size(); j++)
      ip_char[j] = ip_string.at(j);
  ip_char[ip_string.size()] = '\0';
  ipv4.SetBase (ip_char, Ipv4Mask("255.255.255.0"));
  ipv4.Assign (linkServer);

  Ipv4InterfaceContainer serverIpv4; 
  serverIpv4.Add(ipv4.Assign (serverDevices));



  // Create router nodes.
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Add the trace callback function.
  ostringstream oss;
  // oss << "/NodeList/" << router.Get (0) -> GetId () << "/DeviceList/" << linkServer.Get (1)->GetIfIndex() << "/$ns3::PointToPointNetDevice/TxQueue/Enqueue";
  oss << "/NodeList/" << router.Get (0) -> GetId () << "/$ns3::TrafficControlLayer/RootQueueDiscList/" << linkServer.Get (1)->GetIfIndex() << "/Enqueue";
  cout << oss.str() << endl;
  Config::Connect (oss.str(), MakeCallback (&AsciiEnqueueEvent));
  //servers.Get (0)->TraceConnect ("Enqueue", oss.str(), MakeCallback (&AsciiEnqueueEvent));

  oss.str("");
  oss.clear();
  // oss << "/NodeList/" << router.Get (0) -> GetId () << "/DeviceList/" << linkServer.Get (1)->GetIfIndex() << "/$ns3::PointToPointNetDevice/TxQueue/Dequeue";
  oss << "/NodeList/" << router.Get (0) -> GetId () << "/$ns3::TrafficControlLayer/RootQueueDiscList/" << linkServer.Get (1)->GetIfIndex() << "/Dequeue";
  cout << oss.str() << endl;
  Config::Connect (oss.str(), MakeCallback (&AsciiDequeueEvent));
  //servers.Get (0)->TraceConnect ("Enqueue", oss.str(), MakeCallback (&AsciiEnqueueEvent));

  oss.str("");
  oss.clear();
  // oss << "/NodeList/" << router.Get (0) -> GetId () << "/DeviceList/" << linkServer.Get (1)->GetIfIndex() << "/$ns3::PointToPointNetDevice/TxQueue/Drop";
  oss << "/NodeList/" << router.Get (0) -> GetId () << "/$ns3::TrafficControlLayer/RootQueueDiscList/" << linkServer.Get (1)->GetIfIndex() << "/Drop";
  cout << oss.str() << endl;
  Config::Connect (oss.str(), MakeCallback (&AsciiDropEvent));

  oss.str("");
  oss.clear();
  // oss << "/NodeList/" << router.Get (0) -> GetId () << "/DeviceList/" << linkServer.Get (1)->GetIfIndex() << "/$ns3::PointToPointNetDevice/TxQueue/BytesInQueue";
  oss << "/NodeList/" << router.Get (0) -> GetId () << "/$ns3::TrafficControlLayer/RootQueueDiscList/" << linkServer.Get (1)->GetIfIndex() << "/BytesInQueue";
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
  //AsciiTraceHelper ascii;
  //p2p.EnableAsciiAll (ascii.CreateFileStream ("csma-bridge.tr"));

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




 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

 
 







 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
