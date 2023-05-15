#include "../../include/drivers/ggh_driver.hpp"

using namespace Eigen;

Mat GGHDriver::gen_random(int rows, int cols, int range) {
  // generate a random matrix
  CryptoPP::AutoSeededRandomPool rng;
  Mat R(rows, cols);
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
        CryptoPP::Integer comp(rng, -range, range);
        long comp_l = comp.ConvertToLong();
        R(i, j) = comp_l;
    }
  }
  return R;
}

double GGHDriver::hadamard_ratio(Mat M) {
  double ratio = M.determinant();
  for (int i = 0; i < M.rows(); i++) {
    ratio /= M.row(i).norm();
  }
  ratio = std::pow(std::abs(ratio), 1.0/M.rows());
  return ratio;
}

Mat GGHDriver::gen_V() {
  Mat V = gen_random(GGH_N, GGH_N, GGH_D);
  double h = hadamard_ratio(V);
  if (h < 0.82) {
    return gen_V();
  }
  return V;
}

Mat GGHDriver::gen_U() {
  CryptoPP::AutoSeededRandomPool rng;
  Mat U = Mat::Identity(GGH_N, GGH_N);
  for (int r = 0; r < 50; r++) {
    CryptoPP::Integer to(rng, 0, GGH_N-1);
    long k = to.ConvertToLong();
    CryptoPP::Integer from(rng, 0, GGH_N-1);
    long j = from.ConvertToLong();
    if (k != j) {
        CryptoPP::Integer coef(rng, -GGH_DELTA, GGH_DELTA);
        long coef_l = coef.ConvertToLong();
        U.row(k) += U.row(j)*coef_l;
    }

    CryptoPP::Integer swap(rng, 0, (GGH_N-1)*2);
    long swap_l = swap.ConvertToLong();
    j = swap_l % (GGH_N-1);
    if (k != j) {
        Mat tmp = U.row(k);
        U.row(k) = U.row(j);
        U.row(j) = tmp;
        /* if (j != swap_l) {
            U.row(k) *= -1;
        } */
    }
  }
  return U;
}

Mat GGHDriver::babai(Mat V, Mat w) {
  assert(w.rows() == 1);
  w *= V.inverse();
  for (int i = 0; i < w.cols(); i++) {
    w(0, i) = std::roundl(w(0, i));
  }
  w *= V;
  return w;
}

std::pair<Mat, Mat> GGHDriver::GGH_generate() {
  // key generation algorithm
  Mat sk = gen_V();
  Mat U = gen_U();
  Mat pk = U*sk;
  return std::make_pair(sk, pk);
}

Mat GGHDriver::GGH_encrypt(Mat pk, Mat m, std::optional<Mat> rand) {
  assert(m.rows() == 1); // m is a vector
  Mat r;
  if (rand.has_value()) {
    r = rand.value();
  } else {
    r = gen_random(1, GGH_N, GGH_DELTA);
  }
  Mat e = m*pk + r;
  return e;
}

Mat GGHDriver::GGH_decrypt(Mat sk, Mat pk, Mat e) {
  assert(e.rows() == 1); // e is a vector
  Mat v = babai(sk, e);
  Mat m = v*pk.inverse();
  for (int i = 0; i < m.cols(); i++) {
    m(0, i) = std::roundl(m(0, i));
  }
  return m;
}

void eigentest() {
  GGHDriver gghd;
  std::pair<Mat, Mat> keys = gghd.GGH_generate();
  Mat U = gghd.gen_U();
  std::cout << "U " << U << std::endl;
  std::cout << "det " << U.determinant() << std::endl;
  Mat m = gghd.gen_random(1, GGH_N, 100);
  Mat r = gghd.gen_random(1, GGH_N, GGH_DELTA);
  Mat r2 = Mat::Zero(1, GGH_N);
  std::cout << "r " << r << std::endl;
  std::cout << "r2 " << r2 << std::endl;
  Mat enc = gghd.GGH_encrypt(keys.second, m, std::optional<Mat>{r});
  Mat enc2 = gghd.GGH_encrypt(keys.second, m, std::optional<Mat>{r2});
  std::cout << "sk " << keys.first << std::endl;
  Mat dec = gghd.GGH_decrypt(keys.first, keys.second, enc);
  Mat dec2 = gghd.GGH_decrypt(keys.first, keys.second, enc2);
  std::cout << "m    " << m << std::endl;
  std::cout << "dec  " << dec << std::endl;
  std::cout << "dec2 " << dec2 << std::endl;
  std::cout << "bad dec " << gghd.GGH_decrypt(keys.second, keys.second, enc) << std::endl;
  std::cout << "bad dec2 " << gghd.GGH_decrypt(keys.second, keys.second, enc2) << std::endl;
  std::cout << "enc " << enc << std::endl;
  std::cout << "enc2 " << enc2 << std::endl;
  std::cout << gghd.hadamard_ratio(keys.first) << std::endl;
  std::cout << gghd.hadamard_ratio(keys.second) << std::endl;
  Mat V_test(3, 3);
  V_test << -97, 19, 19,
            -36, 30, 86,
            -184, -64, 78;
  Mat U_test(3, 3);
  U_test << 4327, -15447, 23454,
            3297, -11770, 17871,
            5464, -19506, 29617;
  Mat r_test(1, 3);
  r_test << -4, -3, 2;
  Mat m_test(1, 3);
  m_test << 86, -35, -32;
  Mat W_test = U_test*V_test;
}