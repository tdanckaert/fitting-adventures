#include <iostream>

#include <cstring>
#include <vector>
#include "gsl/gsl_multifit_nlin.h"

using std::cout; using std::endl; using std::vector;


class Vector {
public:
  Vector(double *elements, size_t length);
  Vector(vector<double> v);
  Vector(double start, double end, size_t length);
  Vector(size_t length);
  ~Vector();
  gsl_vector *self;
};

Vector::Vector(double *elements, size_t length){
  self = gsl_vector_alloc(length);
  memcpy(self->data, elements, length * sizeof(*(self->data)));
}

Vector::Vector(vector<double> v) {
  self = gsl_vector_alloc(v.size());
  memcpy(self->data, &v[0], v.size() * sizeof(*(self->data)));
}

Vector::Vector(double start, double end, size_t length) {
  self = gsl_vector_alloc(length);
  double step = (end-start)/(length-1);
  for(size_t i=0; i<length; ++i) {
    self->data[i] = start + i*step;
  }
}

Vector::Vector(size_t length) {
  self = gsl_vector_alloc(length);
  gsl_vector_set_zero(self);
}

Vector::~Vector() {
  gsl_vector_free(self);
}

class Solver {
public:
  Solver();
  ~Solver();
  int f(const gsl_vector *X, gsl_vector *f);
  void set();
  void setY(const vector<double>& yVals);
  void solve();
  int df(const gsl_vector *X, gsl_matrix *J);
  //private:
  gsl_multifit_fdfsolver *pSolver;
  const gsl_multifit_fdfsolver_type *type;
  size_t nPoints;
  size_t nParams;
  gsl_multifit_function_fdf gslMultifit;
  Vector initialParams; // todo: better ownership?
  Vector coords;
  Vector y;
};

int gsl_f(const gsl_vector *X, void *solver, gsl_vector *f) {
  return ((Solver*)solver)->f(X, f);
}

int gsl_df(const gsl_vector *X, void *solver, gsl_matrix *J) { 
  return ((Solver*)solver)->df(X, J);
}

int gsl_fdf(const gsl_vector *X, void *solver, gsl_vector *f, gsl_matrix *J) {
  Solver *s=(Solver *)solver;
  int rc = s->f(X,f);
  if (rc != GSL_SUCCESS) {
    return rc;
  }
  rc = s->df(X, J);
  return rc;
}

void Solver::solve() {
  int status=0;
  size_t iter=0;
  do {
    iter++;
    status = gsl_multifit_fdfsolver_iterate(pSolver);
    cout << "status" << gsl_strerror(status) << endl;
    //    print_state(iter, pSolver);
    if(status)
      break;
    status = gsl_multifit_test_delta(pSolver->dx, pSolver->x, 1e-4, 1e-4);
  } while ( status == GSL_CONTINUE && iter < 500);
  cout << "results" << endl
       << "a = " << gsl_vector_get(pSolver->x,1) << endl
       << "b = " << gsl_vector_get(pSolver->x,0) << endl;
  
}

/* f = a*x_i + b, b = X[0], a=X[1] */
int Solver::f(const gsl_vector *X, gsl_vector *f) {
  cout <<  "Solver::f" << endl;
  gsl_vector_memcpy(f, coords.self); // f = x
  gsl_vector_scale(f, gsl_vector_get(X,1));
  gsl_vector_add_constant(f, gsl_vector_get(X,0));
  gsl_vector_sub(f, y.self);
  for(int i=0; i<nPoints; ++i) {
    cout << f->data[i] << endl;
  }
  return GSL_SUCCESS;
}

int Solver::df(const gsl_vector *X, gsl_matrix *J) {
  cout <<  "Solver::df" << endl;
  /* J_ij = df_i/dX_j. -> J_i0 = 1 J_i1 = x_i, */
  gsl_matrix_set_all(J, 1.);
  gsl_matrix_set_col(J, 1, coords.self);
  return GSL_SUCCESS;
}

gsl_multifit_function_fdf gsl_multifit_proto = {
  &gsl_f,
  &gsl_df,
  &gsl_fdf,
  0,
  0,
  NULL
};

// test: fit linear functions at 10 points with 2 params
Solver::Solver() : type(gsl_multifit_fdfsolver_lmsder),
                   nPoints(10),
                   nParams(2),
                   initialParams({0.,0.}),
                   coords(1.,10.,nPoints),
                   y(nPoints) { // todo: somehow initialize with double * to avoid unnecessary object creation ?
  cout << "solver_alloc" << endl;
  for (int i=0; i<nPoints; ++i) {
    y.self->data[i] = 5.*coords.self->data[i] + 3.;
    //    cout << yVals[i] << endl;
  }
  pSolver = gsl_multifit_fdfsolver_alloc(type , nPoints, nParams);
  this->set();
}

void Solver::set() {
  gslMultifit = gsl_multifit_proto;
  gslMultifit.n = nPoints;
  gslMultifit.p = nParams;
  gslMultifit.params = this;
  gsl_multifit_fdfsolver_set(pSolver, &gslMultifit, initialParams.self); // todo: initial params
}

void Solver::setY(const vector<double>& yVals) {
  memcpy(y.self->data, &yVals[0], yVals.size());  
}

Solver::~Solver() {
  cout << "solver_free" << endl;
  gsl_multifit_fdfsolver_free(pSolver);
}

int main() {
  Solver s;
  vector<double> yVals(10);
  s.solve();
  return 0;
}