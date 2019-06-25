#ifndef MYGLOBALROUTEMANAGERIMPL_HPP
#define MYGLOBALROUTEMANAGERIMPL_HPP

#include "ns3/global-route-manager-impl.h"

#include <utility>
#include <vector>
#include <queue>
#include <algorithm>
#include <iostream>
#include "ns3/assert.h"
#include "ns3/fatal-error.h"
#include "ns3/log.h"
#include "ns3/node-list.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/mpi-interface.h"
#include "ns3/global-router-interface.h"
#include "ns3/global-route-manager-impl.h"
#include "ns3/candidate-queue.h"
#include "ns3/ipv4-global-routing.h"

namespace ns3 {

class CandidateQueue;
class Ipv4GlobalRouting;

NS_LOG_COMPONENT_DEFINE( "MyGlobalRouteManagerImpl" );

// ==================================================================== //

class MyGlobalRouteManagerLSDB{
	public:
		MyGlobalRouteManagerLSDB();
		~MyGlobalRouteManagerLSDB();

		void Insert( Ipv4Address addr, GlobalRoutingLSA* lsa );
		GlobalRoutingLSA* GetLSA( Ipv4Address addr ) const;
		GlobalRoutingLSA* GetLSAByLinkData( Ipv4Address addr ) const;
		void Initialize();
		GlobalRoutingLSA* GetExtLSA( uint32_t index ) const;
		uint32_t GetNumExtLSAs() const;

	private:
		typedef std::map<Ipv4Address, GlobalRoutingLSA*> LSDBMap_t;				//!< container of IPv4 addresses / Link State Advertisements
		typedef std::pair<Ipv4Address, GlobalRoutingLSA*> LSDBPair_t;			//!< pair of IPv4 addresses / Link State Advertisements

		LSDBMap_t m_database;																							//!< database of IPv4 addresses / Link State Advertisements
		std::vector<GlobalRoutingLSA*> m_extdatabase;											//!< database of External Link State Advertisements

		MyGlobalRouteManagerLSDB (MyGlobalRouteManagerLSDB& lsdb);
		MyGlobalRouteManagerLSDB& operator= (MyGlobalRouteManagerLSDB& lsdb);
};

// -------------------------------------------------------------------- //

class MyGlobalRouteManagerImpl{
public:
  MyGlobalRouteManagerImpl();
  virtual ~MyGlobalRouteManagerImpl();
  virtual void DeleteGlobalRoutes();
  virtual void BuildGlobalRoutingDatabase();
  virtual void InitializeRoutes();
  void DebugUseLsdb( MyGlobalRouteManagerLSDB* );
  void DebugSPFCalculate( Ipv4Address root );

private:
  MyGlobalRouteManagerImpl( MyGlobalRouteManagerImpl& srmi );
  MyGlobalRouteManagerImpl& operator=( MyGlobalRouteManagerImpl& srmi );

  SPFVertex* m_spfroot; 															//!< the root node
  MyGlobalRouteManagerLSDB* m_lsdb; 									//!< the Link State DataBase (LSDB) of the Global Route Manager

  bool CheckForStubNode( Ipv4Address root );
  void SPFCalculate( Ipv4Address root );
  void SPFProcessStubs( SPFVertex* v );
  void ProcessASExternals( SPFVertex* v, GlobalRoutingLSA* extlsa );
  void SPFNext( SPFVertex* v, CandidateQueue& candidate );
  int SPFNexthopCalculation( SPFVertex* v, SPFVertex* w, GlobalRoutingLinkRecord* l, uint32_t distance );
  void SPFVertexAddParent( SPFVertex* v );
  GlobalRoutingLinkRecord* SPFGetNextLink( SPFVertex* v, SPFVertex* w, GlobalRoutingLinkRecord* prev_link );
  void SPFIntraAddRouter( SPFVertex* v );
  void SPFIntraAddTransit( SPFVertex* v );
  void SPFIntraAddStub( GlobalRoutingLinkRecord *l, SPFVertex* v );
  void SPFAddASExternal( GlobalRoutingLSA *extlsa, SPFVertex *v );
  int32_t FindOutgoingInterfaceId( Ipv4Address a, Ipv4Mask amask = Ipv4Mask( "255.255.255.255" ) );
};

// ==================================================================== //
// ========================== Implementação! ========================== //
// ==================================================================== //

MyGlobalRouteManagerLSDB::MyGlobalRouteManagerLSDB(): m_database(), m_extdatabase(){}

// -------------------------------------------------------------------- //

MyGlobalRouteManagerLSDB::~MyGlobalRouteManagerLSDB(){
  LSDBMap_t::iterator i;

  for( i= m_database.begin() ; i!= m_database.end() ; i++){
    GlobalRoutingLSA* temp = i->second;
    delete temp;
  }
  for( uint32_t j = 0 ; j < m_extdatabase.size() ; j++){
    GlobalRoutingLSA* temp = m_extdatabase.at( j );
    delete temp;
  }

  m_database.clear();
}

// -------------------------------------------------------------------- //

void MyGlobalRouteManagerLSDB::Initialize(){
  LSDBMap_t::iterator i;

  for( i= m_database.begin() ; i!= m_database.end() ; i++ ){
    GlobalRoutingLSA* temp = i->second;
    temp->SetStatus (GlobalRoutingLSA::LSA_SPF_NOT_EXPLORED);
  }
}

// -------------------------------------------------------------------- //

void MyGlobalRouteManagerLSDB::Insert( Ipv4Address addr, GlobalRoutingLSA* lsa ){

  if( lsa->GetLSType() == GlobalRoutingLSA::ASExternalLSAs ){
    m_extdatabase.push_back( lsa );
  } 
  else{
    m_database.insert( LSDBPair_t( addr, lsa ) );
  }
}

// -------------------------------------------------------------------- //

GlobalRoutingLSA* MyGlobalRouteManagerLSDB::GetExtLSA( uint32_t index ) const{

  return m_extdatabase.at (index);
}

// -------------------------------------------------------------------- //

uint32_t MyGlobalRouteManagerLSDB::GetNumExtLSAs() const{

  return m_extdatabase.size ();
}

// -------------------------------------------------------------------- //

GlobalRoutingLSA* MyGlobalRouteManagerLSDB::GetLSA( Ipv4Address addr ) const{

  LSDBMap_t::const_iterator i;
  for( i= m_database.begin() ; i!= m_database.end() ; i++ ){
		if( i->first == addr ){
			return i->second;
		}
	}
  return 0;
}

// -------------------------------------------------------------------- //

GlobalRoutingLSA* MyGlobalRouteManagerLSDB::GetLSAByLinkData( Ipv4Address addr ) const{
  LSDBMap_t::const_iterator i;

  for( i= m_database.begin() ; i!= m_database.end() ; i++ ){
		GlobalRoutingLSA* temp = i->second;

		for( uint32_t j = 0 ; j < temp->GetNLinkRecords() ; j++ ){
			GlobalRoutingLinkRecord *lr = temp->GetLinkRecord( j );
			if( lr->GetLinkType () == GlobalRoutingLinkRecord::TransitNetwork &&	lr->GetLinkData() == addr )
				return temp;
		}
	}

	return 0;
}

// ==================================================================== //

MyGlobalRouteManagerImpl::MyGlobalRouteManagerImpl(): m_spfroot( 0 ){

  m_lsdb = new MyGlobalRouteManagerLSDB ();
}

// -------------------------------------------------------------------- //

MyGlobalRouteManagerImpl::~MyGlobalRouteManagerImpl(){

	if( m_lsdb )
		delete m_lsdb;
}

// -------------------------------------------------------------------- //

void MyGlobalRouteManagerImpl::DebugUseLsdb( MyGlobalRouteManagerLSDB* lsdb ){

  if( m_lsdb )
	  delete m_lsdb;
  m_lsdb = lsdb;
}

// -------------------------------------------------------------------- //

void MyGlobalRouteManagerImpl::DeleteGlobalRoutes(){
  NodeList::Iterator listEnd = NodeList::End ();

  for( NodeList::Iterator i = NodeList::Begin () ; i != listEnd ; i++ ){
    Ptr<Node> node = *i;
    Ptr<GlobalRouter> router = node->GetObject<GlobalRouter>();

    if( router == 0 )
      continue;

    Ptr<Ipv4GlobalRouting> gr = router->GetRoutingProtocol();
    uint32_t j = 0;
    uint32_t nRoutes = gr->GetNRoutes ();

    for( j = 0 ; j < nRoutes ; j++ )
      gr->RemoveRoute( 0 );
  }

	if (m_lsdb){
    delete m_lsdb;
    m_lsdb = new MyGlobalRouteManagerLSDB();
  }
}

// -------------------------------------------------------------------- //

void MyGlobalRouteManagerImpl::BuildGlobalRoutingDatabase(){
  NodeList::Iterator listEnd = NodeList::End();

  for( NodeList::Iterator i = NodeList::Begin() ; i != listEnd ; i++ ){
    Ptr<Node> node = *i;

    Ptr<GlobalRouter> rtr = node->GetObject<GlobalRouter>();
    if( !rtr )
      continue;

    Ptr<Ipv4GlobalRouting> grouting = rtr->GetRoutingProtocol();
    uint32_t numLSAs = rtr->DiscoverLSAs();

    for( uint32_t j = 0 ; j < numLSAs ; ++j ){
      GlobalRoutingLSA* lsa = new GlobalRoutingLSA();
      rtr->GetLSA( j, *lsa );
      NS_LOG_LOGIC( *lsa );

      m_lsdb->Insert( lsa->GetLinkStateId(), lsa ); 
    }
  }
}

// -------------------------------------------------------------------- //

void MyGlobalRouteManagerImpl::InitializeRoutes(){
  NodeList::Iterator listEnd = NodeList::End();

  for( NodeList::Iterator i = NodeList::Begin() ; i != listEnd ; i++ ){
    Ptr<Node> node = *i;

    Ptr<GlobalRouter> rtr = node->GetObject<GlobalRouter>();

    uint32_t systemId = MpiInterface::GetSystemId();
    if( node->GetSystemId() != systemId ) 
      continue;

    if( rtr && rtr->GetNumLSAs() )
      SPFCalculate( rtr->GetRouterId() );
  }
}

// -------------------------------------------------------------------- //

void MyGlobalRouteManagerImpl::SPFNext( SPFVertex* v, CandidateQueue& candidate ){
  SPFVertex* w = 0;
  GlobalRoutingLSA* w_lsa = 0;
  GlobalRoutingLinkRecord *l = 0;
  uint32_t distance = 0;
  uint32_t numRecordsInVertex = 0;

  if( v->GetVertexType() == SPFVertex::VertexRouter )
    numRecordsInVertex = v->GetLSA()->GetNLinkRecords(); 
  if( v->GetVertexType() == SPFVertex::VertexNetwork )
    numRecordsInVertex = v->GetLSA()->GetNAttachedRouters();

  for( uint32_t i = 0 ; i < numRecordsInVertex ; i++ ){

    if( v->GetVertexType() == SPFVertex::VertexRouter ){
      l = v->GetLSA()->GetLinkRecord( i );
      NS_ASSERT( l != 0 );
      if( l->GetLinkType() == GlobalRoutingLinkRecord::StubNetwork )
        continue;

      if( l->GetLinkType() == GlobalRoutingLinkRecord::PointToPoint ){

	      w_lsa = m_lsdb->GetLSA (l->GetLinkId ());
	      NS_ASSERT (w_lsa);
      }
      else if( l->GetLinkType () == GlobalRoutingLinkRecord::TransitNetwork ){
        w_lsa = m_lsdb->GetLSA( l->GetLinkId() );
        NS_ASSERT( w_lsa );
      }
    }

    if( v->GetVertexType() == SPFVertex::VertexNetwork ){
      w_lsa = m_lsdb->GetLSAByLinkData( v->GetLSA()->GetAttachedRouter( i ) );
      if( !w_lsa )
        continue;
    }


    if( w_lsa->GetStatus() == GlobalRoutingLSA::LSA_SPF_IN_SPFTREE )
        continue;

    if( v->GetLSA()->GetLSType() == GlobalRoutingLSA::RouterLSA ){
      NS_ASSERT( l != 0 );
      distance = v->GetDistanceFromRoot() + l->GetMetric();
    }
    else
      distance = v->GetDistanceFromRoot();


    if( w_lsa->GetStatus() == GlobalRoutingLSA::LSA_SPF_NOT_EXPLORED ){

      w = new SPFVertex( w_lsa );
      if( SPFNexthopCalculation( v, w, l, distance ) ){
        w_lsa->SetStatus (GlobalRoutingLSA::LSA_SPF_CANDIDATE);

        candidate.Push (w);
      }
      else
        NS_ASSERT_MSG( 0, "SPFNexthopCalculation never " << "return false, but it does now!" );
    }
    else if( w_lsa->GetStatus() == GlobalRoutingLSA::LSA_SPF_CANDIDATE ){
      SPFVertex* cw;

      cw = candidate.Find( w_lsa->GetLinkStateId() );
      if( cw->GetDistanceFromRoot() < distance )
        continue;

      else if( cw->GetDistanceFromRoot() == distance ){

        w = new SPFVertex( w_lsa );
        SPFNexthopCalculation( v, w, l, distance );
        cw->MergeRootExitDirections( w );
        cw->MergeParent( w );

        SPFVertexAddParent( w );
        delete w;
      }
      else{
        if( SPFNexthopCalculation( v, cw, l, distance ) )
          candidate.Reorder();
      }
    }
  }
}

// -------------------------------------------------------------------- //

int MyGlobalRouteManagerImpl::SPFNexthopCalculation( SPFVertex* v, SPFVertex* w, GlobalRoutingLinkRecord* l, uint32_t distance ){

  if( v == m_spfroot ){
	  if( w->GetVertexType() == SPFVertex::VertexRouter ){
      NS_ASSERT( l );
      GlobalRoutingLinkRecord *linkRemote = 0;
      linkRemote = SPFGetNextLink( w, v, linkRemote );
      Ipv4Address nextHop = linkRemote->GetLinkData();

      uint32_t outIf = FindOutgoingInterfaceId( l->GetLinkData() );

      w->SetRootExitDirection( nextHop, outIf );
      w->SetDistanceFromRoot( distance );
      w->SetParent( v );
    }
    else{
      NS_ASSERT( w->GetVertexType() == SPFVertex::VertexNetwork );

      GlobalRoutingLSA* w_lsa = w->GetLSA();
      NS_ASSERT( w_lsa->GetLSType() == GlobalRoutingLSA::NetworkLSA );

      uint32_t outIf = FindOutgoingInterfaceId( w_lsa->GetLinkStateId(), w_lsa->GetNetworkLSANetworkMask() );

      Ipv4Address nextHop = Ipv4Address::GetZero();
      w->SetRootExitDirection( nextHop, outIf );
      w->SetDistanceFromRoot( distance );
      w->SetParent( v );

      return 1;
    }
  }
  else if( v->GetVertexType() == SPFVertex::VertexNetwork ){
    if( v->GetParent() == m_spfroot ){
      NS_ASSERT( w->GetVertexType() == SPFVertex::VertexRouter );
	    GlobalRoutingLinkRecord *linkRemote = 0;

      while( ( linkRemote = SPFGetNextLink( w, v, linkRemote ) ) ){

        Ipv4Address nextHop = linkRemote->GetLinkData();
        uint32_t outIf = v->GetRootExitDirection().second;
        w->SetRootExitDirection( nextHop, outIf );
      }
    }
    else 
      w->SetRootExitDirection (v->GetRootExitDirection ());
  }
  else
    w->InheritAllRootExitDirections( v );

  w->SetDistanceFromRoot( distance );
  w->SetParent( v );

  return 1;
}

// -------------------------------------------------------------------- //

GlobalRoutingLinkRecord* MyGlobalRouteManagerImpl::SPFGetNextLink( SPFVertex* v, SPFVertex* w, GlobalRoutingLinkRecord* prev_link ){
  bool skip = true;
  bool found_prev_link = false;
  GlobalRoutingLinkRecord* l;

  if( prev_link == 0 ){
    skip = false;
    found_prev_link = true;
  }

  for( uint32_t i = 0 ; i < v->GetLSA ()->GetNLinkRecords() ; ++i ){
    l = v->GetLSA()->GetLinkRecord( i );

    if( l->GetLinkId() == w->GetVertexId() ){
      if( !found_prev_link ){
        found_prev_link = true;
        continue;
      }

      if( skip == false )
        return l;
      else{
        skip = false;
        continue;
      }
    }
  }

  return 0;
}

// -------------------------------------------------------------------- //

void MyGlobalRouteManagerImpl::DebugSPFCalculate( Ipv4Address root ){

  SPFCalculate( root );
}

// -------------------------------------------------------------------- //

bool MyGlobalRouteManagerImpl::CheckForStubNode( Ipv4Address root ){
  GlobalRoutingLSA *rlsa = m_lsdb->GetLSA( root );
  Ipv4Address myRouterId = rlsa->GetLinkStateId();
  int transits = 0;
  GlobalRoutingLinkRecord *transitLink = 0;

  for( uint32_t i = 0 ; i < rlsa->GetNLinkRecords() ; i++ ){
    GlobalRoutingLinkRecord *l = rlsa->GetLinkRecord( i );
    if( l->GetLinkType() == GlobalRoutingLinkRecord::TransitNetwork ){
      transits++;
      transitLink = l;
    }
    else if( l->GetLinkType() == GlobalRoutingLinkRecord::PointToPoint ){
      transits++;
      transitLink = l;
    }
  }
  if( transits == 0 )
    return true;
  if( transits == 1 ){
    if( transitLink->GetLinkType() == GlobalRoutingLinkRecord::TransitNetwork )
      return false;
    else if( transitLink->GetLinkType() == GlobalRoutingLinkRecord::PointToPoint ){
      GlobalRoutingLSA *w_lsa = m_lsdb->GetLSA( transitLink->GetLinkId() );
      uint32_t nLinkRecords = w_lsa->GetNLinkRecords();

      for (uint32_t j = 0 ; j < nLinkRecords ; ++j ){
        GlobalRoutingLinkRecord *lr = w_lsa->GetLinkRecord (j);
        if( lr->GetLinkType() != GlobalRoutingLinkRecord::PointToPoint )
          continue;

        if( lr->GetLinkId () == myRouterId ){
          Ptr<GlobalRouter> router = rlsa->GetNode()->GetObject<GlobalRouter>();
          NS_ASSERT( router );
          Ptr<Ipv4GlobalRouting> gr = router->GetRoutingProtocol();
          NS_ASSERT( gr );
          gr->AddNetworkRouteTo( Ipv4Address( "0.0.0.0" ), Ipv4Mask( "0.0.0.0" ), lr->GetLinkData(), FindOutgoingInterfaceId( transitLink->GetLinkData() ) );

          return true;
				}
    	}
		}
	}

  return false;
}

// -------------------------------------------------------------------- //

void MyGlobalRouteManagerImpl::SPFCalculate( Ipv4Address root ){
  SPFVertex *v;

  m_lsdb->Initialize ();
  CandidateQueue candidate;
  NS_ASSERT (candidate.Size () == 0);

  v = new SPFVertex (m_lsdb->GetLSA (root));

  m_spfroot= v;
  v->SetDistanceFromRoot (0);
  v->GetLSA ()->SetStatus (GlobalRoutingLSA::LSA_SPF_IN_SPFTREE);
  NS_LOG_LOGIC ("Starting SPFCalculate for node " << root);

  if (NodeList::GetNNodes () > 0 && CheckForStubNode (root)){
    delete m_spfroot;
    return;
  }

  for(;;){
    SPFNext (v, candidate);

    if (candidate.Size () == 0)
      break;

    NS_LOG_LOGIC (candidate);
    v = candidate.Pop ();
    NS_LOG_LOGIC ("Popped vertex " << v->GetVertexId ());

    v->GetLSA ()->SetStatus (GlobalRoutingLSA::LSA_SPF_IN_SPFTREE);

    SPFVertexAddParent (v);

    if (v->GetVertexType () == SPFVertex::VertexRouter)
        SPFIntraAddRouter (v);
    else if (v->GetVertexType () == SPFVertex::VertexNetwork)
        SPFIntraAddTransit (v);
    else
        NS_ASSERT_MSG (0, "illegal SPFVertex type");

  }


  SPFProcessStubs (m_spfroot);
  for (uint32_t i = 0; i < m_lsdb->GetNumExtLSAs (); i++){
    m_spfroot->ClearVertexProcessed ();
    GlobalRoutingLSA *extlsa = m_lsdb->GetExtLSA (i);

    ProcessASExternals (m_spfroot, extlsa);
  }

  delete m_spfroot;
  m_spfroot = 0;
}

// -------------------------------------------------------------------- //

void MyGlobalRouteManagerImpl::ProcessASExternals (SPFVertex* v, GlobalRoutingLSA* extlsa){

  if (v->GetVertexType () == SPFVertex::VertexRouter){
    GlobalRoutingLSA *rlsa = v->GetLSA ();

    if ((rlsa->GetLinkStateId ()) == (extlsa->GetAdvertisingRouter ())){
      SPFAddASExternal (extlsa,v);
    }
  }
	for (uint32_t i = 0; i < v->GetNChildren (); i++){
	  if (!v->GetChild (i)->IsVertexProcessed ()){
	    ProcessASExternals (v->GetChild (i), extlsa);
	    v->GetChild (i)->SetVertexProcessed (true);
    }
  }
}

// -------------------------------------------------------------------- //

void MyGlobalRouteManagerImpl::SPFAddASExternal (GlobalRoutingLSA *extlsa, SPFVertex *v){

  NS_ASSERT_MSG (m_spfroot, "GlobalRouteManagerImpl::SPFAddASExternal (): Root pointer not set");

  if (v->GetVertexId () == m_spfroot->GetVertexId ())
      return;

  Ipv4Address routerId = m_spfroot->GetVertexId ();

  NodeList::Iterator i = NodeList::Begin (); 
  NodeList::Iterator listEnd = NodeList::End ();
  for (; i != listEnd; i++){
    Ptr<Node> node = *i;

    Ptr<GlobalRouter> rtr = node->GetObject<GlobalRouter> ();

    if (rtr == 0)
	    continue;

    if (rtr->GetRouterId () == routerId){
      Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
      NS_ASSERT_MSG (ipv4, 
                         "GlobalRouteManagerImpl::SPFIntraAddRouter (): "
                         "QI for <Ipv4> interface failed");

      NS_ASSERT_MSG (v->GetLSA (), 
                     "GlobalRouteManagerImpl::SPFIntraAddRouter (): "
                     "Expected valid LSA in SPFVertex* v");

      Ipv4Mask tempmask = extlsa->GetNetworkLSANetworkMask ();
      Ipv4Address tempip = extlsa->GetLinkStateId ();
      tempip = tempip.CombineMask (tempmask);

      Ptr<GlobalRouter> router = node->GetObject<GlobalRouter> ();
      if (router == 0)
        continue;

      Ptr<Ipv4GlobalRouting> gr = router->GetRoutingProtocol ();
      NS_ASSERT (gr);

      for (uint32_t i = 0; i < v->GetNRootExitDirections (); i++){
        SPFVertex::NodeExit_t exit = v->GetRootExitDirection (i);
        Ipv4Address nextHop = exit.first;
        int32_t outIf = exit.second;

        if (outIf >= 0)
          gr->AddASExternalRouteTo (tempip, tempmask, nextHop, outIf);
      }

	    return;
    }
	}
}

// -------------------------------------------------------------------- //

void MyGlobalRouteManagerImpl::SPFProcessStubs (SPFVertex* v){

  if (v->GetVertexType () == SPFVertex::VertexRouter){
    GlobalRoutingLSA *rlsa = v->GetLSA ();

    for (uint32_t i = 0; i < rlsa->GetNLinkRecords (); i++){
      GlobalRoutingLinkRecord *l = v->GetLSA ()->GetLinkRecord (i);

      if (l->GetLinkType () == GlobalRoutingLinkRecord::StubNetwork){
        SPFIntraAddStub (l, v);
        continue;
      }

    }
  }
  for (uint32_t i = 0 ; i < v->GetNChildren() ; i++ ){
    if (!v->GetChild (i)->IsVertexProcessed ()){
      SPFProcessStubs (v->GetChild (i));
      v->GetChild (i)->SetVertexProcessed (true);
    }
  }
}

// -------------------------------------------------------------------- //

void MyGlobalRouteManagerImpl::SPFIntraAddStub (GlobalRoutingLinkRecord *l, SPFVertex* v){

  NS_ASSERT_MSG (m_spfroot, 
                 "GlobalRouteManagerImpl::SPFIntraAddStub (): Root pointer not set");

  if (v->GetVertexId () == m_spfroot->GetVertexId ())
    return;

  Ipv4Address routerId = m_spfroot->GetVertexId ();

  NodeList::Iterator i = NodeList::Begin (); 
  NodeList::Iterator listEnd = NodeList::End ();
  for (; i != listEnd; i++){
    Ptr<Node> node = *i;

    Ptr<GlobalRouter> rtr = 
      node->GetObject<GlobalRouter> ();

    if (rtr == 0)
      continue;

    if (rtr->GetRouterId () == routerId){
      Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();

      NS_ASSERT_MSG (ipv4, 
                     "GlobalRouteManagerImpl::SPFIntraAddRouter (): "
                     "QI for <Ipv4> interface failed");

      NS_ASSERT_MSG (v->GetLSA (), 
                     "GlobalRouteManagerImpl::SPFIntraAddRouter (): "
                     "Expected valid LSA in SPFVertex* v");

      Ipv4Mask tempmask (l->GetLinkData ().Get ());
      Ipv4Address tempip = l->GetLinkId ();
      tempip = tempip.CombineMask (tempmask);

      Ptr<GlobalRouter> router = node->GetObject<GlobalRouter> ();
      if (router == 0)
        continue;

      Ptr<Ipv4GlobalRouting> gr = router->GetRoutingProtocol ();
      NS_ASSERT (gr);

      for (uint32_t i = 0; i < v->GetNRootExitDirections (); i++){
        SPFVertex::NodeExit_t exit = v->GetRootExitDirection (i);
        Ipv4Address nextHop = exit.first;
        int32_t outIf = exit.second;

        if (outIf >= 0)
          gr->AddNetworkRouteTo (tempip, tempmask, nextHop, outIf);
      }

      return;
    }
  }
}

// -------------------------------------------------------------------- //

int32_t MyGlobalRouteManagerImpl::FindOutgoingInterfaceId (Ipv4Address a, Ipv4Mask amask){

  Ipv4Address routerId = m_spfroot->GetVertexId ();
  NodeList::Iterator i = NodeList::Begin (); 
  NodeList::Iterator listEnd = NodeList::End ();
  for (; i != listEnd; i++){
    Ptr<Node> node = *i;

    Ptr<GlobalRouter> rtr = node->GetObject<GlobalRouter> ();

    if (rtr == 0)
	    continue;

    if (rtr->GetRouterId () == routerId){
      Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
      NS_ASSERT_MSG (ipv4, 
                     "GlobalRouteManagerImpl::FindOutgoingInterfaceId (): "
                     "GetObject for <Ipv4> interface failed");

      int32_t interface = ipv4->GetInterfaceForPrefix (a, amask);

      return interface;
    }
  }

  return -1;
}

// -------------------------------------------------------------------- //

void MyGlobalRouteManagerImpl::SPFIntraAddRouter (SPFVertex* v){

  NS_ASSERT_MSG (m_spfroot, 
                 "GlobalRouteManagerImpl::SPFIntraAddRouter (): Root pointer not set");
  Ipv4Address routerId = m_spfroot->GetVertexId ();

  NodeList::Iterator i = NodeList::Begin (); 
  NodeList::Iterator listEnd = NodeList::End ();
  for (; i != listEnd; i++){
    Ptr<Node> node = *i;
    Ptr<GlobalRouter> rtr = node->GetObject<GlobalRouter> ();

    if (rtr == 0)
      continue;

    if (rtr->GetRouterId () == routerId){
      Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
      NS_ASSERT_MSG (ipv4, 
                     "GlobalRouteManagerImpl::SPFIntraAddRouter (): "
                     "GetObject for <Ipv4> interface failed");

      GlobalRoutingLSA *lsa = v->GetLSA ();
      NS_ASSERT_MSG (lsa, 
                     "GlobalRouteManagerImpl::SPFIntraAddRouter (): "
                     "Expected valid LSA in SPFVertex* v");

      uint32_t nLinkRecords = lsa->GetNLinkRecords ();

      NS_LOG_LOGIC (" Node " << node->GetId () << " found " << nLinkRecords << " link records in LSA " << lsa << "with LinkStateId "<< lsa->GetLinkStateId ());
      for (uint32_t j = 0; j < nLinkRecords; ++j){

        GlobalRoutingLinkRecord *lr = lsa->GetLinkRecord (j);
        if (lr->GetLinkType () != GlobalRoutingLinkRecord::PointToPoint)
	        continue;

        Ptr<GlobalRouter> router = node->GetObject<GlobalRouter> ();
        if (router == 0)
          continue;

        Ptr<Ipv4GlobalRouting> gr = router->GetRoutingProtocol ();
        NS_ASSERT (gr);

        for (uint32_t i = 0; i < v->GetNRootExitDirections (); i++){
          SPFVertex::NodeExit_t exit = v->GetRootExitDirection (i);
          Ipv4Address nextHop = exit.first;
          int32_t outIf = exit.second;

          if (outIf >= 0)
            gr->AddHostRouteTo (lr->GetLinkData (), nextHop, outIf);
	      }
      }

			return;
		}
	}
}

// -------------------------------------------------------------------- //

void MyGlobalRouteManagerImpl::SPFIntraAddTransit (SPFVertex* v){

  NS_ASSERT_MSG (m_spfroot, 
                 "GlobalRouteManagerImpl::SPFIntraAddTransit (): Root pointer not set");

  Ipv4Address routerId = m_spfroot->GetVertexId ();

  NodeList::Iterator i = NodeList::Begin (); 
  NodeList::Iterator listEnd = NodeList::End ();
  for (; i != listEnd; i++){
    Ptr<Node> node = *i;

    Ptr<GlobalRouter> rtr = 
      node->GetObject<GlobalRouter> ();

    if (rtr == 0)
      continue;

    if (rtr->GetRouterId () == routerId){
      NS_LOG_LOGIC ("setting routes for node " << node->GetId ());

      Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
      NS_ASSERT_MSG (ipv4, 
                     "GlobalRouteManagerImpl::SPFIntraAddTransit (): "
                     "GetObject for <Ipv4> interface failed");

      GlobalRoutingLSA *lsa = v->GetLSA ();
      NS_ASSERT_MSG (lsa, 
                     "GlobalRouteManagerImpl::SPFIntraAddTransit (): "
                     "Expected valid LSA in SPFVertex* v");

      Ipv4Mask tempmask = lsa->GetNetworkLSANetworkMask ();
      Ipv4Address tempip = lsa->GetLinkStateId ();
      tempip = tempip.CombineMask (tempmask);
      Ptr<GlobalRouter> router = node->GetObject<GlobalRouter> ();

      if (router == 0)
        continue;

      Ptr<Ipv4GlobalRouting> gr = router->GetRoutingProtocol ();
      NS_ASSERT (gr);

      for( uint32_t i = 0; i < v->GetNRootExitDirections (); i++ ){
        SPFVertex::NodeExit_t exit = v->GetRootExitDirection (i);
        Ipv4Address nextHop = exit.first;
        int32_t outIf = exit.second;

        if (outIf >= 0)
          gr->AddNetworkRouteTo (tempip, tempmask, nextHop, outIf);
      }
    }
  } 
}

// -------------------------------------------------------------------- //

void MyGlobalRouteManagerImpl::SPFVertexAddParent (SPFVertex* v){

  for (uint32_t i=0;;){
    SPFVertex* parent;

    if ((parent = v->GetParent (i++)) == 0)
			break;
    parent->AddChild (v);
  }
}

// ==================================================================== //

} // namespace ns3

#endif
