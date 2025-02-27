#include "PolynomialList.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cmath>


using namespace std;

PolynomialList::PolynomialList(const PolynomialList& other)
    : m_Polynomial(other.m_Polynomial) {}

PolynomialList::PolynomialList(const string& file) {
    if (!ReadFromFile(file)) {
        throw runtime_error("Failed to read polynomial from file: " + file);
    }
}

PolynomialList::PolynomialList(const double* cof, const int* deg, int n) {
    for (int i = 0; i < n; ++i){
        if(cof[i] != 0){
            AddOneTerm(Term(deg[i], cof[i]));
        }
    }
}

PolynomialList::PolynomialList(const vector<int>& deg, const vector<double>& cof) {
    size_t n = min(deg.size(), cof.size());
    for (size_t i = 0; i < n; ++i) {
        if (cof[i] != 0.0) {
            AddOneTerm(Term(deg[i], cof[i]));
        }
    }
}

double PolynomialList::coff(int deg) const {
    for(const auto& term : m_Polynomial){
        if(term.deg == deg){
            return term.cof;
        }else if(term.deg < deg){
            break;
        }
    }
    return 0.; // you should return a correct value
}

double& PolynomialList::coff(int deg) {
    for(auto it = m_Polynomial.begin(); it!= m_Polynomial.end(); ++it){
        if(it->deg == deg){
            return it->cof;
        }else if(it->deg < deg){
            return m_Polynomial.insert(it, Term(deg, 0.0))->cof;
        }
    }

    m_Polynomial.push_back(Term(deg, 0.0));
    return m_Polynomial.back().cof;
}

void PolynomialList::compress() {
    auto it = m_Polynomial.begin();
    while (it!= m_Polynomial.end()) {
        if (it->cof == 0) {
            it = m_Polynomial.erase(it);
        } else {
            ++it;
        }
    }
}

PolynomialList PolynomialList::operator+(const PolynomialList& right) const {
    PolynomialList result(*this);
    for (const auto& term : right.m_Polynomial) {
        result.AddOneTerm(term);
    }
    result.compress();
    return result;
}

PolynomialList PolynomialList::operator-(const PolynomialList& right) const {
    PolynomialList result(*this);
    for(const auto& term : right.m_Polynomial){
        result.AddOneTerm(Term(term.deg, -term.cof));
    }
    result.compress();
    return result;
}

PolynomialList PolynomialList::operator*(const PolynomialList& right) const {
    PolynomialList result;
    for (const auto& t1 : m_Polynomial) {
        for (const auto& t2 : right.m_Polynomial) {
            result.AddOneTerm(Term(
                t1.deg + t2.deg,  
                t1.cof * t2.cof   
            ));
        }
    }
    result.compress();
    return result;
}

PolynomialList& PolynomialList::operator=(const PolynomialList& right) {
    if (this != &right) {
        m_Polynomial = right.m_Polynomial;
    }    
    return *this;
}

void PolynomialList::Print() const {
    if (m_Polynomial.empty()) {
        cout << "0" << endl;
        return;
    }

    bool firstTerm = true;
    for (const auto& term : m_Polynomial) {
        if (firstTerm) {
            if (term.cof < 0) cout << "-";
            firstTerm = false;
        } else {
            cout << (term.cof > 0 ? " + " : " - ");
        }

        const double absCof = abs(term.cof);
        if (absCof != 1.0 || term.deg == 0) {
            cout << absCof;
        }

        if (term.deg > 0) {
            cout << "x";
            if (term.deg > 1) {
                cout << "^" << term.deg;
            }
        }
    }
    cout << endl;
}

bool PolynomialList::ReadFromFile(const string& file) {
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
            AddOneTerm(Term(deg, cof));
        }
    }
    return true;
}

PolynomialList::Term& PolynomialList::AddOneTerm(const Term& term) {
    if (term.cof == 0.0) return *m_Polynomial.end();  

    auto it = m_Polynomial.begin();
    while (it != m_Polynomial.end()) {
        if (it->deg == term.deg) {  
            it->cof += term.cof;
            if (it->cof == 0.0) {    
                it = m_Polynomial.erase(it);
                return *it;  
            }
            return *it;
        } else if (it->deg < term.deg) {
            break;
        }
        ++it;
    }

    return *m_Polynomial.insert(it, term);
}
