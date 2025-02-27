#include "PolynomialMap.h"
#include <iostream>
#include <fstream>
#include <cassert>
#include <cmath>
#include <sstream>

using namespace std;

PolynomialMap::PolynomialMap(const PolynomialMap& other) {
    m_Polynomial = other.m_Polynomial;
}

PolynomialMap::PolynomialMap(const double* cof, const int* deg, int n) {
    for (int i = 0; i < n; ++i) {
        if (cof[i] != 0.0) {
            m_Polynomial[deg[i]] += cof[i];
        }
    }
    compress();
}

PolynomialMap::PolynomialMap(const vector<int>& deg, const vector<double>& cof) {
    assert(deg.size() == cof.size());
    for (size_t i = 0; i < deg.size(); ++i) {
        if (cof[i] != 0.0) {
            m_Polynomial[deg[i]] += cof[i];
        }
    }
    compress();
}

double PolynomialMap::coff(int i) const {
    auto it = m_Polynomial.find(i);
    return (it != m_Polynomial.end()) ? it->second : 0.0;
}

double& PolynomialMap::coff(int i) {
    return m_Polynomial[i];
}

void PolynomialMap::compress() {
    auto it = m_Polynomial.begin();
    while (it != m_Polynomial.end()) {
        if (it->second == 0.0) {
            it = m_Polynomial.erase(it);
        } else {
            ++it;
        }
    }
}

PolynomialMap PolynomialMap::operator+(const PolynomialMap& right) const {
    PolynomialMap result(*this);
    for (const auto& term : right.m_Polynomial) {
        result.m_Polynomial[term.first] += term.second;
    }
    result.compress();
    return result;
}

PolynomialMap PolynomialMap::operator-(const PolynomialMap& right) const {
    PolynomialMap result(*this);
    for (const auto& term : right.m_Polynomial) {
        result.m_Polynomial[term.first] -= term.second;
    }
    result.compress();
    return result;
}

PolynomialMap PolynomialMap::operator*(const PolynomialMap& right) const {
    PolynomialMap result;
    for (const auto& term1 : m_Polynomial) {
        for (const auto& term2 : right.m_Polynomial) {
            int deg = term1.first + term2.first;
            double cof = term1.second * term2.second;
            result.m_Polynomial[deg] += cof;
        }
    }
    result.compress();
    return result;
}

PolynomialMap& PolynomialMap::operator=(const PolynomialMap& right) {
    if (this != &right) {
        m_Polynomial = right.m_Polynomial;
    }
    return *this;
}

void PolynomialMap::Print() const {
    if (m_Polynomial.empty()) {
        cout << "0" << endl;
        return;
    }

    bool firstTerm = true;
    for (const auto& term : m_Polynomial) {
        double cof = term.second;
        int deg = term.first;

        if (cof == 0.0) continue;

        if (firstTerm) {
            firstTerm = false;
            if (cof < 0) cout << "-";
        } else {
            cout << (cof > 0 ? " + " : " - ");
        }

        double absCof = abs(cof);
        if (absCof != 1.0 || deg == 0) {
            cout << absCof;
        }

        if (deg > 0) {
            cout << "x";
            if (deg > 1) {
                cout << "^" << deg;
            }
        }
    }
    cout << endl;
}

bool PolynomialMap::ReadFromFile(const string& file) {
    m_Polynomial.clear();
    ifstream in(file);
    if (!in.is_open()) return false;

    string line;
    if (!getline(in, line)) return false;

    istringstream header(line);
    char p;
    int numTerms;
    if (!(header >> p >> numTerms) || p != 'P') return false;

    for (int i = 0; i < numTerms; ++i) {
        if (!getline(in, line)) return false;

        istringstream termStream(line);
        int deg;
        double cof;
        if (!(termStream >> deg >> cof)) return false;

        if (cof != 0.0) {
            m_Polynomial[deg] += cof;
        }
    }

    compress();
    return true;
}