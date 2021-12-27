/*************************************************************************************

Grid physics library, www.github.com/paboyle/Grid

Source file: ./lib/qcd/modules/plaquette.h

Copyright (C) 2017

Author: Guido Cossu <guido.cossu@ed.ac.uk>
Author: Christopher Kelly <ckelly@bnl.gov>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

See the full license in the file "LICENSE" in the top level distribution
directory
*************************************************************************************/
			   /*  END LEGAL */

#pragma once

NAMESPACE_BEGIN(Grid);

template <class Gimpl>
class WilsonFlow: public Smear<Gimpl>{
  unsigned int Nstep;
  unsigned int measure_interval;
  mutable RealD epsilon, taus;


  mutable WilsonGaugeAction<Gimpl> SG;

  void evolve_step(typename Gimpl::GaugeField&) const;
  void evolve_step_adaptive(typename Gimpl::GaugeField&, RealD);
  RealD tau(unsigned int t)const {return epsilon*(t+1.0); }

public:
  INHERIT_GIMPL_TYPES(Gimpl)

  explicit WilsonFlow(unsigned int Nstep, RealD epsilon, unsigned int interval = 1):
  Nstep(Nstep),
    epsilon(epsilon),
    measure_interval(interval),
    SG(WilsonGaugeAction<Gimpl>(3.0)) {
    // WilsonGaugeAction with beta 3.0
    assert(epsilon > 0.0);
    LogMessage();
  }

  void LogMessage() {
    std::cout << GridLogMessage
	      << "[WilsonFlow] Nstep   : " << Nstep << std::endl;
    std::cout << GridLogMessage
	      << "[WilsonFlow] epsilon : " << epsilon << std::endl;
    std::cout << GridLogMessage
	      << "[WilsonFlow] full trajectory : " << Nstep * epsilon << std::endl;
  }

  virtual void smear(GaugeField&, const GaugeField&) const;

  virtual void derivative(GaugeField&, const GaugeField&, const GaugeField&) const {
    assert(0);
    // undefined for WilsonFlow
  }

  void smear_adaptive(GaugeField&, const GaugeField&, RealD maxTau);

  //Compute t^2 <E(t)> for timestep 'step' from the plaquette
  RealD energyDensityPlaquette(unsigned int step, const GaugeField& U) const;

  //Compute t^2 <E(t)> for time taus, set by smear_adaptive, from the plaquette
  RealD energyDensityPlaquette(const GaugeField& U) const;
  
  //Evolve the gauge field by Nstep steps of epsilon and return the energy density computed every interval steps
  //The smeared field is output as V
  std::vector<RealD> flowMeasureEnergyDensityPlaquette(GaugeField &V, const GaugeField& U) const;

  std::vector<RealD> flowMeasureEnergyDensityPlaquette(const GaugeField& U) const;

  //Compute t^2 <E(t)> for time from the 1x1 cloverleaf form
  //t is the Wilson flow time
  RealD energyDensityCloverleaf(RealD t, const GaugeField& U) const;

  //Evolve the gauge field by Nstep steps of epsilon and return the Cloverleaf energy density computed every interval steps
  //The smeared field is output as V
  std::vector<RealD> flowMeasureEnergyDensityCloverleaf(GaugeField &V, const GaugeField& U) const;

  std::vector<RealD> flowMeasureEnergyDensityCloverleaf(const GaugeField& U) const;
};


////////////////////////////////////////////////////////////////////////////////
// Implementations
////////////////////////////////////////////////////////////////////////////////
template <class Gimpl>
void WilsonFlow<Gimpl>::evolve_step(typename Gimpl::GaugeField &U) const{
  GaugeField Z(U.Grid());
  GaugeField tmp(U.Grid());
  SG.deriv(U, Z);
  Z *= 0.25;                                  // Z0 = 1/4 * F(U)
  Gimpl::update_field(Z, U, -2.0*epsilon);    // U = W1 = exp(ep*Z0)*W0

  Z *= -17.0/8.0;
  SG.deriv(U, tmp); Z += tmp;                 // -17/32*Z0 +Z1
  Z *= 8.0/9.0;                               // Z = -17/36*Z0 +8/9*Z1
  Gimpl::update_field(Z, U, -2.0*epsilon);    // U_= W2 = exp(ep*Z)*W1

  Z *= -4.0/3.0;
  SG.deriv(U, tmp); Z += tmp;                 // 4/3*(17/36*Z0 -8/9*Z1) +Z2
  Z *= 3.0/4.0;                               // Z = 17/36*Z0 -8/9*Z1 +3/4*Z2
  Gimpl::update_field(Z, U, -2.0*epsilon);    // V(t+e) = exp(ep*Z)*W2
}

template <class Gimpl>
void WilsonFlow<Gimpl>::evolve_step_adaptive(typename Gimpl::GaugeField &U, RealD maxTau) {
  if (maxTau - taus < epsilon){
    epsilon = maxTau-taus;
  }
  //std::cout << GridLogMessage << "Integration epsilon : " << epsilon << std::endl;
  GaugeField Z(U.Grid());
  GaugeField Zprime(U.Grid());
  GaugeField tmp(U.Grid()), Uprime(U.Grid());
  Uprime = U;
  SG.deriv(U, Z);
  Zprime = -Z;
  Z *= 0.25;                                  // Z0 = 1/4 * F(U)
  Gimpl::update_field(Z, U, -2.0*epsilon);    // U = W1 = exp(ep*Z0)*W0

  Z *= -17.0/8.0;
  SG.deriv(U, tmp); Z += tmp;                 // -17/32*Z0 +Z1
  Zprime += 2.0*tmp;
  Z *= 8.0/9.0;                               // Z = -17/36*Z0 +8/9*Z1
  Gimpl::update_field(Z, U, -2.0*epsilon);    // U_= W2 = exp(ep*Z)*W1
    

  Z *= -4.0/3.0;
  SG.deriv(U, tmp); Z += tmp;                 // 4/3*(17/36*Z0 -8/9*Z1) +Z2
  Z *= 3.0/4.0;                               // Z = 17/36*Z0 -8/9*Z1 +3/4*Z2
  Gimpl::update_field(Z, U, -2.0*epsilon);    // V(t+e) = exp(ep*Z)*W2

  // Ramos 
  Gimpl::update_field(Zprime, Uprime, -2.0*epsilon); // V'(t+e) = exp(ep*Z')*W0
  // Compute distance as norm^2 of the difference
  GaugeField diffU = U - Uprime;
  RealD diff = norm2(diffU);
  // adjust integration step
    
  taus += epsilon;
  //std::cout << GridLogMessage << "Adjusting integration step with distance: " << diff << std::endl;
    
  epsilon = epsilon*0.95*std::pow(1e-4/diff,1./3.);
  //std::cout << GridLogMessage << "New epsilon : " << epsilon << std::endl;

}

template <class Gimpl>
RealD WilsonFlow<Gimpl>::energyDensityPlaquette(unsigned int step, const GaugeField& U) const {
  RealD td = tau(step);
  return 2.0 * td * td * SG.S(U)/U.Grid()->gSites();
}

template <class Gimpl>
RealD WilsonFlow<Gimpl>::energyDensityPlaquette(const GaugeField& U) const {
  return 2.0 * taus * taus * SG.S(U)/U.Grid()->gSites();
}

template <class Gimpl>
std::vector<RealD> WilsonFlow<Gimpl>::flowMeasureEnergyDensityPlaquette(GaugeField &V, const GaugeField& U) const{
  std::vector<RealD> out;
  V = U;
  for (unsigned int step = 0; step < Nstep; step++) { //bn  tau = epsilon*(step+1) so tau after performing step=0 is epsilon
    std::cout << GridLogMessage << "[WilsonFlow] Evolving step " << step << std::endl;
    evolve_step(V);
    if( step % measure_interval == 0){
      std::cout << GridLogMessage << "[WilsonFlow] Computing energy density for step " << step << std::endl;
      out.push_back( energyDensityPlaquette(step,V) );
    }
  }
  return out;
}

template <class Gimpl>
std::vector<RealD> WilsonFlow<Gimpl>::flowMeasureEnergyDensityPlaquette(const GaugeField& U) const{
  GaugeField V(U);
  return flowMeasureEnergyDensityPlaquette(V,U);
}

//Compute t^2 <E(t)> for time from the 1x1 cloverleaf form
template <class Gimpl>
RealD WilsonFlow<Gimpl>::energyDensityCloverleaf(RealD t, const GaugeField& U) const{
  typedef typename Gimpl::GaugeLinkField GaugeMat;
  typedef typename Gimpl::GaugeField GaugeLorentz;

  assert(Nd == 4);
  //E = 1/2 tr( F_munu F_munu )
  //However as  F_numu = -F_munu, only need to sum the trace of the squares of the following 6 field strengths:
  //F_01 F_02 F_03   F_12 F_13  F_23
  GaugeMat F(U.Grid());
  LatticeComplexD R(U.Grid());
  R = Zero();
  
  for(int mu=0;mu<3;mu++){
    for(int nu=mu+1;nu<4;nu++){
      WilsonLoops<Gimpl>::FieldStrength(F, U, mu, nu);
      R = R + trace(F*F);
    }
  }
  ComplexD out = sum(R);
  out = t*t*out / RealD(U.Grid()->gSites());
  return -real(out); //minus sign necessary for +ve energy
}

template <class Gimpl>
std::vector<RealD> WilsonFlow<Gimpl>::flowMeasureEnergyDensityCloverleaf(GaugeField &V, const GaugeField& U) const{
  std::vector<RealD> out;
  V = U;
  for (unsigned int step = 0; step < Nstep; step++) { //bn  tau = epsilon*(step+1) so tau after performing step=0 is epsilon
    std::cout << GridLogMessage << "[WilsonFlow] Evolving step " << step << std::endl;
    evolve_step(V);
    if( step % measure_interval == 0){
      std::cout << GridLogMessage << "[WilsonFlow] Computing Cloverleaf energy density for step " << step << std::endl;
      out.push_back( energyDensityCloverleaf(tau(step),V) );
    }
  }
  return out;
}

template <class Gimpl>
std::vector<RealD> WilsonFlow<Gimpl>::flowMeasureEnergyDensityCloverleaf(const GaugeField& U) const{
  GaugeField V(U);
  return flowMeasureEnergyDensityCloverleaf(V,U);
}



//#define WF_TIMING 
template <class Gimpl>
void WilsonFlow<Gimpl>::smear(GaugeField& out, const GaugeField& in) const {
  out = in;
  for (unsigned int step = 1; step <= Nstep; step++) {
    auto start = std::chrono::high_resolution_clock::now();
    evolve_step(out);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
#ifdef WF_TIMING
    std::cout << "Time to evolve " << diff.count() << " s\n";
#endif
    std::cout << GridLogMessage << "[WilsonFlow] Energy density (plaq) : "
		  << step << "  " << tau(step) << "  " 
	      << energyDensityPlaquette(step,out) << std::endl;
    if( step % measure_interval == 0){
      std::cout << GridLogMessage << "[WilsonFlow] Top. charge           : "
		<< step << "  " 
		<< WilsonLoops<Gimpl>::TopologicalCharge(out) << std::endl;
    }
  }
}

template <class Gimpl>
void WilsonFlow<Gimpl>::smear_adaptive(GaugeField& out, const GaugeField& in, RealD maxTau){
  out = in;
  taus = epsilon;
  unsigned int step = 0;
  do{
    step++;
    //std::cout << GridLogMessage << "Evolution time :"<< taus << std::endl;
    evolve_step_adaptive(out, maxTau);
    std::cout << GridLogMessage << "[WilsonFlow] Energy density (plaq) : "
		  << step << "  " << taus << "  "
	      << energyDensityPlaquette(out) << std::endl;
    if( step % measure_interval == 0){
      std::cout << GridLogMessage << "[WilsonFlow] Top. charge           : "
		<< step << "  " 
		<< WilsonLoops<Gimpl>::TopologicalCharge(out) << std::endl;
    }
  } while (taus < maxTau);



}

NAMESPACE_END(Grid);

