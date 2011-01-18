// Copyright (C) 2010 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#include "Common/CBuilder.hpp"
#include "Common/OptionT.hpp"
#include "Common/OptionURI.hpp"

#include "Mesh/CList.hpp"
#include "Mesh/CElements.hpp"

#include "Solver/Actions/CLoopOperation.hpp"


/////////////////////////////////////////////////////////////////////////////////////

using namespace CF::Common;
using namespace CF::Mesh;

namespace CF {
namespace Solver {
namespace Actions {

///////////////////////////////////////////////////////////////////////////////////////

CLoopOperation::CLoopOperation ( const std::string& name ) : 
  Common::CAction(name),
  m_call_config_elements(true),
  m_idx(0)
{  
  // Following option is ignored if the loop is not about elements
  m_properties.add_option< OptionURI > ("Elements","Elements that are being looped", URI("cpath:"));
  m_properties["Elements"].as_option().attach_trigger ( boost::bind ( &CLoopOperation::config_elements,   this ) );
  
  m_properties.add_option< OptionT<Uint> > ("LoopIndex","Index that is being looped", 0u );
  m_properties["LoopIndex"].as_option().link_to( &m_idx );
  
}

////////////////////////////////////////////////////////////////////////////////

void CLoopOperation::config_elements()
{
  // Safeguard in case elements are set using set_elements
  // otherwise this would get triggered
  if (m_call_config_elements)
  {
    URI uri;
    property("Elements").put_value(uri);
    m_elements = Core::instance().root()->look_component<CElements>(uri);
    if ( is_null(m_elements.lock()) )
      throw CastingFailed (FromHere(), "Elements must be of a CElements or derived type");    
  }
}

////////////////////////////////////////////////////////////////////////////////

void CLoopOperation::set_elements(CElements& elements)
{
  // disable CLoopOperation::config_elements() trigger
  m_call_config_elements = false;
  
  // Set elements
  m_elements = elements.as_type<CElements>();
  
  // Call triggers
  property("Elements").as_option().trigger();
  
  // re-enable CLoop::Operation::config_elements() trigger
  m_call_config_elements = true;
}

////////////////////////////////////////////////////////////////////////////////////

} // Actions
} // Solver
} // CF

////////////////////////////////////////////////////////////////////////////////////
