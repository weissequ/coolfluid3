// Copyright (C) 2010 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#ifndef CF_RDM_Schemes_RKLDA_hpp
#define CF_RDM_Schemes_RKLDA_hpp

#include "RDM/RDSolver.hpp"
#include "RDM/IterativeSolver.hpp"
#include "RDM/TimeStepping.hpp"

#include "RDM/CellTerm.hpp"
#include "RDM/SchemeBase.hpp"

#include "RDM/Schemes/LibSchemes.hpp"

namespace CF {
namespace RDM {

/////////////////////////////////////////////////////////////////////////////////////

class RDM_SCHEMES_API RKLDA : public RDM::CellTerm {

public: // typedefs

  /// the actual scheme implementation is a nested class
  /// varyng with shape function (SF), quadrature rule (QD) and Physics (PHYS)
  template < typename SF, typename QD, typename PHYS > class Term;

  typedef boost::shared_ptr< RKLDA > Ptr;
  typedef boost::shared_ptr< RKLDA const > ConstPtr;

public: // functions

  /// Contructor
  /// @param name of the component
  RKLDA ( const std::string& name );

  /// Virtual destructor
  virtual ~RKLDA();

  /// Get the class name
  static std::string type_name () { return "RKLDA"; }

  /// Execute the loop for all elements
  virtual void execute();

};

/////////////////////////////////////////////////////////////////////////////////////


template < typename SF, typename QD, typename PHYS >
class RDM_SCHEMES_API RKLDA::Term : public SchemeBase<SF,QD,PHYS> {

public: // typedefs

  /// base class type
  typedef SchemeBase<SF,QD,PHYS> B;

  /// pointers
  typedef boost::shared_ptr< Term > Ptr;
  typedef boost::shared_ptr< Term const> ConstPtr;

public: // functions

  /// Contructor
  /// @param name of the component
  Term ( const std::string& name ) : SchemeBase<SF,QD,PHYS>(name)
  {
    regist_typeinfo(this);

    for(Uint n = 0; n < SF::nb_nodes; ++n)
      DvPlus[n].setZero();

    m_options["Elements"].attach_trigger ( boost::bind ( &RKLDA::Term<SF,QD,PHYS>::config_coeffs, this ) );

    // definition of the alpha and beta coefficients of RK method

    /// @warning RKLDA only supports up to RK3

    #define RK_MAX 3

    rkalphas(RK_MAX,RK_MAX-1);

    rkbetas(RK_MAX,RK_MAX);

    rkcoeff(RK_MAX-1,RK_MAX-1);

    switch( RK_MAX )
    {
    case 1: /// @todo Forward Euler

      rkalphas(0,0) = 1.;

      rkbetas(0,0) =  0.;

      break;

    case 2:

      rkalphas(0,0) =  0.5;
      rkalphas(1,0) =  0.5;

      rkcoeff(0,0) =   1.0;

      rkbetas (0,0) =  0.0;
      rkbetas (0,1) = -1.0;
      rkbetas (1,0) =  0.0;
      rkbetas (1,1) =  1.0;

      break;

    case 3:

      rkalphas(0,0) = 0.25  ;
      rkalphas(1,0) = 0.25  ;
      rkalphas(2,0) = 0.    ;
      rkalphas(0,1) = 1./6. ;
      rkalphas(1,1) = 1./6. ;
      rkalphas(2,1) = 2./3. ;

      rkcoeff(0,0) =    1.0 ;
      rkcoeff(1,0) =    0.0 ;
      rkcoeff(0,1) =    0.5 ;
      rkcoeff(1,1) =    2.0 ;

      rkbetas(0,0) = 0.  ;
      rkbetas(0,1) = -0.5 ;
      rkbetas(0,2) = -2. ;
      rkbetas(1,0) = 0. ;
      rkbetas(1,1) = 0. ;
      rkbetas(1,2) = 0. ;
      rkbetas(2,0) = 0. ;
      rkbetas(2,1) = 0.5 ;
      rkbetas(2,2) = 2. ;

      break ;

//    default:

    }


  }

  /// Get the class name
  static std::string type_name () { return "RKLDA.Scheme<" + SF::type_name() + ">"; }

  /// execute the action
  virtual void execute ();

protected: // helper function

  void config_coeffs()
  {
    RDSolver& mysolver = this->parent().as_type<CellTerm>().solver().as_type<RDSolver>();
    rkorder = mysolver.properties().template value<Uint>("rkorder");
    step    = mysolver.iterative_solver().properties().template value<Uint>("iteration");
    dt      = mysolver.time_stepping().get_child("Time").option("time_step").template value<Real>();
  }

protected: // data

  Uint rkorder; ///< order of the RK method
  Uint step;    ///< current RK step
  Real dt;      ///< time step size

  RealMatrix rkalphas;  ///< matrix with alpha coefficients of RK method
  RealMatrix rkbetas;   ///< matrix with beta  coefficients of RK method
  RealMatrix rkcoeff;   ///< matrix with the coefficients that multiply du_s

  /// The operator L in the advection equation Lu = f
  /// Matrix Ki_n stores the value L(N_i) at each quadrature point for each shape function N_i
  typename B::PhysicsMT  Ki_n [SF::nb_nodes];
  /// sum of Lplus to be inverted
  typename B::PhysicsMT  sumLplus;
  /// inverse Ki+ matix
  typename B::PhysicsMT  InvKi_n;
  /// right eigen vector matrix
  typename B::PhysicsMT  Rv;
  /// left eigen vector matrix
  typename B::PhysicsMT  Lv;
  /// diagonal matrix with eigen values
  typename B::PhysicsVT  Dv;
  /// temporary hold of Values of the operator L(u) computed in quadrature points.
  typename B::PhysicsVT  LUwq;
  /// diagonal matrix with positive eigen values
  typename B::PhysicsVT  DvPlus [SF::nb_nodes];

};

/////////////////////////////////////////////////////////////////////////////////////

template<typename SF,typename QD, typename PHYS>
void RKLDA::Term<SF,QD,PHYS>::execute()
{

// get element connectivity

const Mesh::CTable<Uint>::ConstRow nodes_idx = this->connectivity_table->array()[B::idx()];

#if 0
    //compute delta u_s

    for ( Uint s = 0 ; s < SF::nb_nodes ; ++s) // nb_states = number of solution points
    {
        du[s] = 0. ;
        for ( Uint r = 0 ; r < rkorder ; ++r)
        {
            for ( Uint eq = 0 ; eq < PHYS::MODEL::_neqs; ++eq)
                du[s][eq] += rkbetas(r,step) * kstates ( eq , r );
        }
    }
#endif

    if (step > 1)
    {
        // add mass matrix contribution
        for (Uint n=0; n<SF::nb_nodes; ++n)
        {
            for (Uint eq=0; eq < PHYS::MODEL::_neqs; ++eq)
            {
                for ( Uint p=0; p<PHYS::MODEL::_neqs ; ++p)
                {
                    if ( p==eq )
                    {
                        (*B::residual)[nodes_idx[n]][eq] = 1. / 12. * 2. * rkcoeff(step - 2 , rkorder - 2);// * du[eq];
                    }
                    else
                    {
                        (*B::residual)[nodes_idx[n]][eq] = 1. / 12. * 1. * rkcoeff(step - 2 , rkorder - 2);// * du[eq];
                    }
                }
            }
        }
    }
        // there isn't a way to compute it automatically, yet
        // it's necessary to define s
        /// @todo lumped and not lumped in the same line

    for ( Uint r = 0 ; r < step ; ++r)
    {

        // change current solution pointer

        B::interpolate( nodes_idx );

        // L(N)+ @ each quadrature point

        for(Uint q = 0 ; q < QD::nb_points ; ++q) // loop over each quadrature point
        {
            // compute the contributions to phi_i
            B::sol_gradients_at_qdpoint(q);

            PHYS::compute_properties(B::X_q.row(q),
                                     B::U_q.row(q),
                                     B::dUdXq,
                                     B::phys_props);

            for( Uint n = 0 ; n < SF::nb_nodes ; ++n)
            {
              B::dN[XX] = B::dNdX[XX](q,n);
              B::dN[YY] = B::dNdX[YY](q,n);

              PHYS::flux_jacobian_eigen_structure(B::phys_props,
                                             B::dN,
                                             Rv,
                                             Lv,
                                             Dv );

              // diagonal matrix of positive eigen values

              DvPlus[n] = Dv.unaryExpr(std::ptr_fun(plus));

              Ki_n[n] = Rv * DvPlus[n].asDiagonal() * Lv;
            }

            // compute L(u)

            PHYS::residual(B::phys_props,
                     B::dFdU,
                     B::LU );

            // compute L(N)+

            sumLplus = Ki_n[0];
            for(Uint n = 1 ; n < SF::nb_nodes ; ++n)
              sumLplus += Ki_n[n];

              // invert the sum L plus

            InvKi_n = sumLplus.inverse();

            // compute the phi_i LDA integral

            LUwq = InvKi_n * B::LU * B::wj[q];

            for(Uint n = 0 ; n < SF::nb_nodes ; ++n)
              B::Phi_n.row(n) +=  Ki_n[n] * LUwq;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // save the phi_i
                // save B::Phi_n.row
                //Phin.row() += Phi_n.row(n) ;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // compute contribution to wave_speed

            for(Uint n = 0 ; n < SF::nb_nodes ; ++n)
              (*B::wave_speed)[nodes_idx[n]][0] += DvPlus[n].maxCoeff() * B::wj[q];
        }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // update the residual (according to k)
        if ( step > 1)
        {
            for (Uint n=0; n<SF::nb_nodes; ++n)
            {
              for (Uint eq=0; eq < PHYS::MODEL::_neqs; ++eq)
                (*B::residual)[nodes_idx[n]][eq] += rkalphas(r,step - 2) * dt * B::Phi_n(n,eq);
            }
        }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }
}
//#endif

#if 0
    // get element connectivity

  const Mesh::CTable<Uint>::ConstRow nodes_idx = this->connectivity_table->array()[B::idx()];

  B::interpolate( nodes_idx );

  // L(N)+ @ each quadrature point

  for(Uint q=0; q < QD::nb_points; ++q)
  {

    B::sol_gradients_at_qdpoint(q);

    PHYS::compute_properties(B::X_q.row(q),
                             B::U_q.row(q),
                             B::dUdXq,
                             B::phys_props);

    for(Uint n=0; n < SF::nb_nodes; ++n)
    {
      B::dN[XX] = B::dNdX[XX](q,n);
      B::dN[YY] = B::dNdX[YY](q,n);

      PHYS::flux_jacobian_eigen_structure(B::phys_props,
                                     B::dN,
                                     Rv,
                                     Lv,
                                     Dv );

      // diagonal matrix of positive eigen values

      DvPlus[n] = Dv.unaryExpr(std::ptr_fun(plus));

      Ki_n[n] = Rv * DvPlus[n].asDiagonal() * Lv;
    }

    // compute L(u)

    PHYS::residual(B::phys_props,
             B::dFdU,
             B::LU );

    // compute L(N)+

    sumLplus = Ki_n[0];
    for(Uint n = 1; n < SF::nb_nodes; ++n)
      sumLplus += Ki_n[n];

    // invert the sum L plus

    InvKi_n = sumLplus.inverse();

    // compute the phi_i LDA intergral

    LUwq = InvKi_n * B::LU * B::wj[q];

    for(Uint n = 0; n < SF::nb_nodes; ++n)
      B::Phi_n.row(n) +=  Ki_n[n] * LUwq;

    // compute the wave_speed for scaling the update

    for(Uint n = 0; n < SF::nb_nodes; ++n)
      (*B::wave_speed)[nodes_idx[n]][0] += DvPlus[n].maxCoeff() * B::wj[q];


  } // loop qd points

  // update the residual

  for (Uint n=0; n<SF::nb_nodes; ++n)
    for (Uint v=0; v < PHYS::MODEL::_neqs; ++v)
      (*B::residual)[nodes_idx[n]][v] += B::Phi_n(n,v);

#endif
  // debug

  //  std::cout << "LDA ELEM [" << idx() << "]" << std::endl;
  //  std::cout << "  Operator:" << std::endl;
  //  std::cout << "               Sum of weights = " << m_quadrature.weights.sum() << std::endl;
  //  std::cout << "               Jacobians in physical space:" << std::endl;
  //  std::cout << jacob << std::endl;
  //  std::cout << "LDA: Area = " << wj.sum() << std::endl;


  //    std::cout << "X    [" << q << "] = " << X_q.row(q)    << std::endl;
  //    std::cout << "U    [" << q << "] = " << U_q.row(q)     << std::endl;
  //    std::cout << "dUdX[XX] [" << q << "] = " << dUdX[XX].row(q)       << std::endl;
  //    std::cout << "dUdX[YY] [" << q << "] = " << dUdX[YY].row(q)       << std::endl;
  //    std::cout << "LU   [" << q << "] = " << Lu.transpose() << std::endl;
  //    std::cout << "wj   [" << q << "] = " << wj[q]             << std::endl;
  //    std::cout << "--------------------------------------"     << std::endl;

  //  std::cout << "nodes_idx";
  //  for ( Uint i = 0; i < nodes_idx.size(); ++i)
  //     std::cout << " " << nodes_idx[i];

  //  std::cout << "mesh::fill function" <<  std::endl;
  //  std::cout << "nodes: " << nodes << std::endl;

  //  std::cout << "solution: " << U_n << std::endl;
  //  std::cout << "phi: " << Phi_n << std::endl;

  //  std::cout << " AREA : " << wj.sum() << std::endl;

  //  std::cout << "phi [";

  //  for (Uint n=0; n < SF::nb_nodes; ++n)
  //    for (Uint v=0; v < PHYS::MODEL::_neqs; ++v)
  //      std::cout << Phi_n(n,v) << " ";
  //  std::cout << "]" << std::endl;

  //    if( idx() > 2 ) exit(0);



/////////////////////////////////////////////////////////////////////////////////////

} // RDM
} // CF

#endif // CF_RDM_Schemes_RKLDA_hpp