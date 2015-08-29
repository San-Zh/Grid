#include "Grid.h"


using namespace std;
using namespace Grid;
using namespace Grid::QCD;

int main (int argc, char ** argv)
{
  Grid_init(&argc,&argv);

  std::vector<int> latt_size   = GridDefaultLatt();
  std::vector<int> simd_layout = GridDefaultSimd(4,vComplex::Nsimd());
  std::vector<int> mpi_layout  = GridDefaultMpi();
  
  GridCartesian            Fine(latt_size,simd_layout,mpi_layout);
  GridRedBlackCartesian  RBFine(latt_size,simd_layout,mpi_layout);
  GridParallelRNG  pRNG(&Fine);

  std::vector<int> seeds({5,6,7,8});
  pRNG.SeedFixedIntegers(seeds);
  LatticeLorentzColourMatrix     U(&Fine);

  SU3::HotConfiguration(pRNG, U);

  // simplify template declaration? Strip the lorentz from the second template
  WilsonGaugeAction<LatticeLorentzColourMatrix, LatticeColourMatrix> Waction(5.6);

  Real mass=-0.77;
  WilsonFermionR FermOp(U,Fine,RBFine,mass);
  
  ConjugateGradient<LatticeFermion>  CG(1.0e-8,10000);
  
  TwoFlavourPseudoFermionAction<WilsonImplR> WilsonNf2(FermOp,CG,CG);
  
  //Collect actions
  ActionLevel Level1(1);
  ActionLevel Level2(4);
  Level1.push_back(&WilsonNf2);
  Level1.push_back(&Waction);
  //  Level1.push_back(&Waction);

  ActionSet FullSet;
  FullSet.push_back(Level1);
  //  FullSet.push_back(Level2);

  // Create integrator
  typedef MinimumNorm2  IntegratorAlgorithm;// change here to change the algorithm
  //  typedef LeapFrog  IntegratorAlgorithm;// change here to change the algorithm
  IntegratorParameters MDpar(12,20,1.0);
  Integrator<IntegratorAlgorithm> MDynamics(&Fine,MDpar, FullSet);

  // Create HMC
  HMCparameters HMCpar;
  HybridMonteCarlo<IntegratorAlgorithm>  HMC(HMCpar, MDynamics);

  HMC.evolve(U);

}
