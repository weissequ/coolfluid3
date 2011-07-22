// Copyright (C) 2010 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#include "Common/CBuilder.hpp"
#include "Common/OptionArray.hpp"
#include "Common/Foreach.hpp"

#include "Mesh/CField.hpp"
#include "Mesh/CMesh.hpp"

#include "RDM/Cleanup.hpp"

/////////////////////////////////////////////////////////////////////////////////////

using namespace CF::Common;
using namespace CF::Mesh;

namespace CF {
namespace RDM {


///////////////////////////////////////////////////////////////////////////////////////

Common::ComponentBuilder < Cleanup, CAction, LibRDM > Cleanup_Builder;

///////////////////////////////////////////////////////////////////////////////////////

Cleanup::Cleanup ( const std::string& name ) : Solver::Action(name)
{
  mark_basic();

  std::vector< URI > dummy0;
  m_options.add_option< OptionArrayT < URI > > ("Fields", dummy0)
      ->set_description("Fields to cleanup")
      ->attach_trigger ( boost::bind ( &Cleanup::config_fields,   this ) );

  std::vector< std::string > dummy1;
  m_options.add_option( OptionArrayT<std::string>::create("FieldTags", dummy1))
      ->set_description("Tags of the field for which to apply the action")
      ->attach_trigger ( boost::bind ( &Cleanup::config_field_tags,   this ) );
}



void Cleanup::config_fields()
{
  std::vector<URI> vec; option("Fields").put_value(vec);

  boost_foreach(const URI field_path, vec)
  {
    Component& comp = access_component(field_path);

    if ( CField::Ptr field = comp.as_ptr<CField>() )
    {
      boost::weak_ptr<CField> wptr = field;
      m_fields.push_back( wptr );
    }
    else
      throw ValueNotFound ( FromHere(), "Could not find field with path [" + field_path.path() +"]" );
  }
}


void Cleanup::config_field_tags()
{
  std::vector<std::string> vec; option("FieldTags").put_value(vec);

	RDSolver& mysolver = solver().as_type<RDSolver>();

  boost_foreach(const std::string tag, vec)
    boost_foreach( CLink& link, find_components_with_tag<CLink>(mysolver.fields()) )
    {
			if( CField::Ptr field = link.follow()->as_ptr<CField>() )
			{
        boost::weak_ptr<CField> wptr = field;
        m_fields.push_back( wptr );
			}
	  }	
}



void Cleanup::execute()
{
  // loop over fields to cleanup
  boost_foreach(boost::weak_ptr<CField> ptr, m_fields)
  {
    if( ptr.expired() ) continue; // skip if pointer invalid

    CTable<Real>& field = ptr.lock()->data();

    field = 0.; // set all entries to zero
  }
}

////////////////////////////////////////////////////////////////////////////////


} // RDM
} // CF
