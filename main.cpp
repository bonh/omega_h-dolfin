#include <Omega_h_dolfin.hpp>
#include <Omega_h_library.hpp>
#include <Omega_h_mesh.hpp>
#include <Omega_h_file.hpp>
#include <Omega_h_class.hpp>
#include <Omega_h_adapt.hpp>
#include <Omega_h_build.hpp>
#include "Poisson.h"

class Source : public dolfin::Expression
{
  void eval(dolfin::Array<double>& values, const dolfin::Array<double>& x) const
  {
    double dx = x[0] - 0.5;
    double dy = x[1] - 0.5;
    values[0] = 10*exp(-(dx*dx + dy*dy) / 0.02);
  }
};

class dUdN : public dolfin::Expression
{
  void eval(dolfin::Array<double>& values, const dolfin::Array<double>& x) const
  {
    values[0] = sin(5*x[0]);
  }
};

class DirichletBoundary : public dolfin::SubDomain
{
  bool inside(const dolfin::Array<double>& x, bool on_boundary) const
  {
    return x[0] < DOLFIN_EPS or x[0] > 1.0 - DOLFIN_EPS;
  }
};

int main(int argc, char** argv)
{
  auto lib_osh = Omega_h::Library(&argc, &argv);
  auto comm_osh = lib_osh.world();

  auto mesh_osh = Omega_h::build_box(comm_osh,
      1.0, 1.0, 0.0, 32, 32, 0);

#ifdef OMEGA_H_USE_MPI
  auto mesh_dolfin = std::make_shared<dolfin::Mesh>(comm_osh->get_impl());
#else
  auto mesh_dolfin = std::make_shared<dolfin::Mesh>();
#endif
  Omega_h::to_dolfin(*mesh_dolfin, &mesh_osh);

  dolfin::File file_dolfin("dolfin.pvd");
  Omega_h::vtk::Writer file_osh("omega_h", &mesh_osh);

  file_dolfin << *mesh_dolfin;
  file_osh.write();

  auto V = std::make_shared<Poisson::FunctionSpace>(mesh_dolfin);

  auto u0 = std::make_shared<dolfin::Constant>(0.0);
  auto boundary = std::make_shared<DirichletBoundary>();
  dolfin::DirichletBC bc(V, u0, boundary);

  Poisson::BilinearForm a(V, V);
  Poisson::LinearForm L(V);
  auto f = std::make_shared<Source>();
  auto g = std::make_shared<dUdN>();
  L.f = f;
  L.g = g;

  dolfin::Function u(V);
  solve(a == L, u, bc);

  Omega_h::from_dolfin(&mesh_osh, u, "u");

  file_dolfin << u;
  file_osh.write();

//Omega_h::MetricInput metric_input;
//auto source = Omega_h::MetricSource(OMEGA_H_VARIATION, 1e-3, "u");
//metric_input.sources.push_back(source);
//metric_input.should_limit_lengths = true;
//metric_input.min_length = 1.0 / 32.0;
//metric_input.max_length = 1.0 / 2.0;
//metric_input.should_limit_gradation = true;
//Omega_h::generate_target_metric_tag(&mesh_osh, metric_input);
//Omega_h::add_implied_metric_tag(&mesh_osh);
//Omega_h::vtk::write_vtu("before.vtu", &mesh_osh);
//Omega_h::AdaptOpts opts(&mesh_osh);
//opts.xfer_opts.type_map["u"] = OMEGA_H_LINEAR_INTERP;
//while (Omega_h::approach_metric(&mesh_osh, opts)) {
//  Omega_h::adapt(&mesh_osh, opts);
//}
//Omega_h::vtk::write_vtu("after.vtu", &mesh_osh);

  // Plot solution
//plot(u);
//interactive();

  return 0;
}
