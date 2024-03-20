/* ******************************************************************
***
**    OpenSees - Open System for Earthquake Engineering Simulation **
**          Pacific Earthquake Engineering Research Center **
** **
** **
** (C) Copyright 1999, The Regents of the University of California **
** All Rights Reserved. **
** **
** Commercial use of this program without express permission of the **
** University of California, Berkeley, is strictly prohibited.  See **
** file 'COPYRIGHT'  in main directory for information on usage and **
** redistribution,  and for a DISCLAIMER OF ALL WARRANTIES. **
** **
** Developed by: **
**   Frank McKenna (fmckenna@ce.berkeley.edu) **
**   Gregory L. Fenves (fenves@ce.berkeley.edu) **
**   Filip C. Filippou (filippou@ce.berkeley.edu) **
** **
** ******************************************************************
*/

// Minjie
#include "CurvedPipe.h"

#include <CrdTransf.h>
#include <LinearCrdTransf3d.h>

#include <cmath>

// 20 gauss points in (wi, xi)
std::vector<double> CurvedPipe::gaussPts = {
    0.1527533871307258,  -0.0765265211334973, 0.1527533871307258,
    0.0765265211334973,  0.1491729864726037,  -0.2277858511416451,
    0.1491729864726037,  0.2277858511416451,  0.1420961093183820,
    -0.3737060887154195, 0.1420961093183820,  0.3737060887154195,
    0.1316886384491766,  -0.5108670019508271, 0.1316886384491766,
    0.5108670019508271,  0.1181945319615184,  -0.6360536807265150,
    0.1181945319615184,  0.6360536807265150,  0.1019301198172404,
    -0.7463319064601508, 0.1019301198172404,  0.7463319064601508,
    0.0832767415767048,  -0.8391169718222188, 0.0832767415767048,
    0.8391169718222188,  0.0626720483341091,  -0.9122344282513259,
    0.0626720483341091,  0.9122344282513259,  0.0406014298003869,
    -0.9639719272779138, 0.0406014298003869,  0.9639719272779138,
    0.0176140071391521,  -0.9931285991850949, 0.0176140071391521,
    0.9931285991850949};

// function object for

void *OPS_CurvedPipeElement() {
    // check inputs
    if (OPS_GetNumRemainingInputArgs() < 8) {
        opserr << "Invalid #args,  want: element CurvedPipe "
                  "tag? nd1? nd2? pipeMatTag? pipeSecTag?"
                  "xC? yC? zC?"
                  "<-T0 T0? -p p? -cMass? -tolWall? tolWall?>\n";
        return 0;
    }

    // get tag
    int iData[5];
    int numData = 5;
    if (OPS_GetIntInput(&numData, iData) < 0) {
        opserr << "WARNING invalid integer input for curved pipe "
                  "element\n";
        return 0;
    }

    // get center
    Vector center(3);
    numData = 3;
    if (OPS_GetDoubleInput(&numData, &center(0)) < 0) {
        opserr << "WARNING invalid center or radius input for curved "
                  "pipe element\n";
        return 0;
    }

    // get data
    double T0 = 0.0, pressure = 0.0;
    double tolWall = 0.1;
    int cMass = 0;
    numData = 1;
    while (OPS_GetNumRemainingInputArgs() > 0) {
        const char *theType = OPS_GetString();
        if (strcmp(theType, "-T0") == 0) {
            if (OPS_GetNumRemainingInputArgs() > 0) {
                if (OPS_GetDoubleInput(&numData, &T0) < 0) {
                    opserr << "WARNING: failed to read T0\n";
                    return 0;
                }
            }
        } else if (strcmp(theType, "-p") == 0) {
            if (OPS_GetNumRemainingInputArgs() > 0) {
                if (OPS_GetDoubleInput(&numData, &pressure) < 0) {
                    opserr << "WARNING: failed to read internal "
                              "pressure\n";
                    return 0;
                }
            }
        } else if (strcmp(theType, "-tolWall") == 0) {
            if (OPS_GetNumRemainingInputArgs() > 0) {
                if (OPS_GetDoubleInput(&numData, &tolWall) < 0) {
                    opserr << "WARNING: failed to read fraction of "
                              "wall thickness "
                              "to be used for dimensional tolerance "
                              "tests\n";
                    return 0;
                }
            }
            if (tolWall < 0 || tolWall > 1) {
                opserr << "WARNING: tolWall < 0 or > 1\n";
                return 0;
            }
        } else if (strcmp(theType, "-cMass") == 0) {
            cMass = 1;
        }
    }

    auto *theSect = dynamic_cast<PipeSection *>(
        OPS_getSectionForceDeformation(iData[4]));
    if (theSect == 0) {
        opserr << "WARNING: section " << iData[4]
               << " is not found or not a curved pipe section\n";
        return 0;
    }

    auto *theMat = dynamic_cast<PipeMaterial *>(
        OPS_getUniaxialMaterial(iData[3]));
    if (theMat == 0) {
        opserr << "WARNING: uniaxialMaterial " << iData[3]
               << " is not found or not a curved pipe material\n";
        return 0;
    }

    auto *ele = new CurvedPipe(iData[0], iData[1], iData[2], *theMat,
                               *theSect, center, T0, pressure, cMass);

    return ele;
}

CurvedPipe::CurvedPipe()
    : Pipe(), center(3), radius(0.0), theta0(0.0), tolWall(0.1) {}

CurvedPipe::CurvedPipe(int tag, int nd1, int nd2, PipeMaterial &mat,
                       PipeSection &sect, const Vector &c, double to,
                       double pre, int cm, double tol)
    : Pipe(tag, ELE_TAG_CurvedPipe),
      center(3),
      radius(0.0),
      theta0(0.0),
      tolWall(tol) {
    if (Pipe::createPipe(nd1, nd2, mat, sect, cm, 0, 0, pre) < 0) {
        opserr << "WARNING: failed to create curved pipe element\n";
        exit(-1);
    }
    if (center.Size() != 3) {
        center.resize(3);
    }
    for (int i = 0; i < c.Size(); ++i) {
        if (i >= 3) {
            break;
        }
        center(i) = c(i);
    }
}

CurvedPipe::~CurvedPipe() {}

const char *CurvedPipe::getClassType(void) const {
    return "CurvedPipe";
};

void CurvedPipe::setDomain(Domain *theDomain) {
    // create transf3D internally
    if (theDomain == 0) {
        opserr << "Pipe::setDomain -- Domain is null\n";
        exit(-1);
    }

    int ndm = OPS_GetNDM();
    if (ndm != 3) {
        opserr << "WARNING: pipe element must be 3D\n";
        exit(-1);
    }

    theNodes[0] = theDomain->getNode(connectedExternalNodes(0));
    theNodes[1] = theDomain->getNode(connectedExternalNodes(1));

    if (theNodes[0] == 0) {
        opserr << "Pipe::setDomain  tag: " << this->getTag()
               << " -- Node 1: " << connectedExternalNodes(0)
               << " does not exist\n";
        exit(-1);
    }

    if (theNodes[1] == 0) {
        opserr << "Pipe::setDomain  tag: " << this->getTag()
               << " -- Node 2: " << connectedExternalNodes(1)
               << " does not exist\n";
        exit(-1);
    }

    const auto &crdsI = theNodes[0]->getCrds();
    const auto &crdsJ = theNodes[1]->getCrds();

    Vector CI = crdsI;
    CI -= center;
    CI.Normalize();

    Vector IJ = crdsJ;
    IJ -= crdsI;
    IJ.Normalize();

    Vector zAxis;
    if (crossProduct(CI, IJ, zAxis) < 0) {
        exit(-1);
    }

    if (ElasticBeam3d::theCoordTransf != 0) {
        delete ElasticBeam3d::theCoordTransf;
        ElasticBeam3d::theCoordTransf = 0;
    }
    ElasticBeam3d::theCoordTransf =
        new LinearCrdTransf3d(nextTransfTag(), zAxis);
    if (ElasticBeam3d::theCoordTransf == 0) {
        opserr << "WARNING: failed to crete Transformation object -- "
                  "CurvedPipe\n";
        exit(-1);
    }

    // update section data
    if (updateSectionData() < 0) {
        opserr << "Pipe::setDomain failed to update section data\n";
        return;
    }

    // update material data
    if (updateMaterialData() < 0) {
        opserr << "Pipe::setDomain failed to update material data\n";
        return;
    }

    // set domain
    this->ElasticBeam3d::setDomain(theDomain);

    // compute theta0
    if (this->getTheta0() < 0) {
        opserr << "WARNING: failed to compute theta0\n";
        exit(-1);
    }
}

const Matrix &CurvedPipe::getTangentStiff() {
    // ub
    const Vector &v = theCoordTransf->getBasicTrialDisp();

    // kb and pb0
    Matrix kbm;
    Vector pb0;
    if (kb(kbm, pb0) < 0) {
        opserr
            << "WARNING: failed to compute kb -- getTangentStiff\n";
        ElasticBeam3d::K.Zero();
        return ElasticBeam3d::K;
    }

    // pb
    q.addMatrixVector(0.0, kbm, v, 1.0);
    q += pb0;

    return theCoordTransf->getGlobalStiffMatrix(kbm, q);
}

const Matrix &CurvedPipe::getInitialStiff() {
    // kb and pb0
    Matrix kbm;
    Vector pb0;
    if (kb(kbm, pb0) < 0) {
        opserr
            << "WARNING: failed to compute kb -- getInitialStiff\n";
        ElasticBeam3d::K.Zero();
        return ElasticBeam3d::K;
    }

    return theCoordTransf->getInitialGlobalStiffMatrix(kbm);
}

void CurvedPipe::zeroLoad(void) {
    // update section data
    if (updateSectionData() < 0) {
        opserr << "CurvedPipe::setDomain failed to update section "
                  "data\n";
        return;
    }

    // update material data
    if (updateMaterialData() < 0) {
        opserr << "CurvedPipe::setDomain failed to update material "
                  "data\n";
        return;
    }

    this->ElasticBeam3d::zeroLoad();
}

const Vector &CurvedPipe::getResistingForce() {
    // ub
    const Vector &v = theCoordTransf->getBasicTrialDisp();

    // kb and pb0
    Matrix kbm;
    Vector pb0;
    if (kb(kbm, pb0) < 0) {
        opserr
            << "WARNING: failed to compute kb -- getResistingForce\n";
        ElasticBeam3d::P.Zero();
        return ElasticBeam3d::P;
    }

    // pb
    q.addMatrixVector(0.0, kbm, v, 1.0);
    q += pb0;

    Vector p0Vec;
    plw(p0Vec);

    P = theCoordTransf->getGlobalResistingForce(q, p0Vec);

    // subtract external load P = P - Q
    if (rho != 0) {
        P.addVector(1.0, Q, -1.0);
    }

    return P;
}

int CurvedPipe::getTheta0() {
    // compute radius
    const auto &crds1 = theNodes[0]->getCrds();
    const auto &crds2 = theNodes[1]->getCrds();

    double R1 = (center - crds1).Norm();
    double R2 = (center - crds2).Norm();
    radius = (R1 + R2) / 2.0;
    if (radius <= 0) {
        opserr << "WARNING: radius <= 0\n";
        return -1;
    }

    double thk = theSect->WALL();
    if (fabs(R1 - R2) > tolWall * thk) {
        opserr << "WARNING: the computed radius from node I is "
                  "different to "
                  "the one computed from node J more than "
               << tolWall << " * wall thickness\n";
        return -1;
    }

    // compute theta0
    double Lhalf = theCoordTransf->getInitialLength() / 2.0;
    if (Lhalf > 0.99985 * radius) {
        opserr << "WARNING: the angle of the curve >= 178 degree\n";
        return -1;
    }
    theta0 = asin(Lhalf / radius);

    return 0;
}

void CurvedPipe::bx(double theta, Matrix &mat) {
    mat.resize(6, 6);
    mat.Zero();
    double c = cos(theta);
    double s = sin(theta);
    double L = theCoordTransf->getInitialLength();
    double R = radius;
    double H = R * (c - cos(theta0));
    double H0 = R * cos(theta0);
    double invL = 1.0 / L;
    mat(0, 0) = c;
    mat(0, 1) = -s * invL;
    mat(0, 2) = -s * invL;
    mat(1, 0) = -H;
    mat(1, 1) = R * s * invL - 0.5;
    mat(1, 2) = R * s * invL + 0.5;
    mat(2, 3) = s * H0 * invL - 0.5 * c;
    mat(2, 4) = s * H0 * invL + 0.5 * c;
    mat(2, 5) = -s;
    mat(3, 3) = R * invL * (1 - cos(theta - theta0));
    mat(3, 4) = R * invL * (1 - cos(theta + theta0));
    mat(3, 5) = c;
    mat(4, 0) = s;
    mat(4, 1) = c * invL;
    mat(4, 2) = c * invL;
    mat(5, 3) = invL;
    mat(5, 4) = invL;
}

void CurvedPipe::Spx(double theta, Vector &vec) {
    vec.resize(6);
    vec.Zero();
    double wx = ElasticBeam3d::wx;
    double wy = ElasticBeam3d::wy;
    double wz = ElasticBeam3d::wz;
    double c = cos(theta);
    double s = sin(theta);
    double L = theCoordTransf->getInitialLength();
    double R = radius;
    double R2 = R * R;
    double H0 = R * cos(theta0);
    double invL = 1.0 / L;
    double stt0 = sin(theta - theta0);
    double ctt0 = cos(theta - theta0);
    vec(0) = wx * R *
                 (c * theta0 - s - c * theta +
                  2 * s * theta0 * H0 * invL) -
             wy * R * s * theta;
    vec(1) = wx * R *
                 (c * R * theta - 2 * s * R * theta0 * H0 * invL -
                  c * R * theta0 + theta0 * H0) +
             wy * R * (s * R * theta - theta0 * L * 0.5 + c * R - H0);
    vec(2) = wz * R2 * (-theta0 * stt0 + ctt0 - 1);
    vec(3) = wz * R2 * (-theta + theta0 * ctt0 + stt0);
    vec(4) = wx * R *
                 (c + s * theta0 - s * theta -
                  2 * c * theta0 * H0 * invL) +
             wy * R * c * theta;
    vec(5) = -wz * R * theta;
}

void CurvedPipe::fs(double theta, Matrix &mat) {
    mat.resize(6, 6);
    mat.Zero();

    mat(0, 0) = 1.0 / (E * A);
    mat(1, 1) = 1.0 / (E * Iz);
    mat(2, 2) = 1.0 / (E * Iy);
    mat(3, 3) = 1.0 / (G * Jx);

    double alphaV = theSect->ALFAV();
    if (alphaV > 99) {
        alphaV = 0.0;
    }
    if (alphaV > 0) {
        double Ay = A / alphaV;
        double Az = A / alphaV;
        mat(4, 4) = 1.0 / (G * Ay);
        mat(5, 5) = 1.0 / (G * Az);
    }
}

void CurvedPipe::fb(double theta, Matrix &mat) {
    mat.resize(6, 6);
    mat.Zero();

    Matrix bxmat, fsmat;
    bx(theta, bxmat);
    fs(theta, fsmat);
    mat.addMatrixTripleProduct(0.0, bxmat, fsmat, 1.0);
}

void CurvedPipe::ubno(double theta, Vector &vec) {
    vec.resize(6);
    vec.Zero();

    Matrix fsmat, bxmat;
    Vector spxvec;
    fs(theta, fsmat);
    bx(theta, bxmat);
    Spx(theta, spxvec);

    Vector temp(6);
    temp.addMatrixVector(0.0, fsmat, spxvec, 1.0);
    vec.addMatrixTransposeVector(0.0, bxmat, temp, 1.0);
}

int CurvedPipe::kb(Matrix &mat, Vector &vec) {
    Matrix fbmat;
    Vector ubnovec;

    // integrate fb and ub0
    integrateGauss(-theta0, theta0, fbmat, ubnovec);

    // thermal and pressure added to ub0
    // update section data
    if (updateSectionData() < 0) {
        opserr << "Pipe::setDomain failed to update section data\n";
        return -1;
    }

    // update material data
    if (updateMaterialData() < 0) {
        opserr << "Pipe::setDomain failed to update material data\n";
        return -1;
    }

    // due to thermal
    double dT = aveTemp();
    double R = radius;
    if (dT > 0) {
        ubnovec(0) += 2 * R * alp * dT * sin(theta0);
    }

    // due to internal pressure
    if (pressure != 0) {
        double dout = theSect->DOUT();
        double thk = theSect->WALL();

        // converted from SAP 5
        double RM = (dout - thk) * 0.5;
        double DU2 = R / RM;
        double DUM = pressure * RM * 0.5 / (E * thk);
        double DU3 = 1.0 + DUM * (1 - nu * (2 * DU2 - 1) / (DU2 - 1));
        double BTA = DU3 / (1.0 + DUM * (2 - nu));
        BTA = -(1.0 - BTA) / R;

        ubnovec(0) += 0.5 * pressure * R * (dout - thk) *
                      (1. - 2 * nu) * sin(theta0) / (E * thk);
        ubnovec(0) +=
            2 * R * R * BTA * (theta0 * cos(theta0) - sin(theta0));
        ubnovec(1) += -R * BTA * theta0;
        ubnovec(2) += R * BTA * theta0;
    }

    // kb
    if (fbmat.Invert(mat) < 0) {
        return -1;
    }

    // pb0
    vec.resize(6);
    vec.addMatrixVector(0.0, mat, ubnovec, -1.0);

    return 0;
}

void CurvedPipe::integrateGauss(double a, double b, Matrix &resm,
                                Vector &resv) {
    double p = (b - a) / 2.0;
    double q = (b + a) / 2.0;
    resm.resize(6, 6);
    resm.Zero();
    resv.resize(6);
    resv.Zero();
    for (int i = 0; i < (int)gaussPts.size() / 2; ++i) {
        double xi = gaussPts[2 * i + 1] * p + q;
        double wi = gaussPts[2 * i] * p;
        Matrix mat;
        Vector vec;
        fb(xi, mat);
        resm.addMatrix(1.0, mat, wi);
        ubno(xi, vec);
        resv.addVector(1.0, vec, wi);
    }
    resm *= radius;
    resv *= radius;
}

void CurvedPipe::plw(Vector &vec) {
    // [plw1, plw2, plw8, plw3, plw9, plw4] match
    // LinearCrdTransf3D::getGlobalResistingForce
    vec.resize(6);

    // member load
    double wx = ElasticBeam3d::wx;
    double wy = ElasticBeam3d::wy;
    double wz = ElasticBeam3d::wz;
    double R = radius;
    double H0 = R * cos(theta0);
    double L = theCoordTransf->getInitialLength();

    // plw1
    vec(0) = -2 * wx * R * theta0;

    // plw2
    vec(1) = wx * R * (1.0 - 2 * theta0 * H0 / L) - wy * R * theta0;

    // plw8
    vec(2) = -wx * R * (1.0 - 2 * theta0 * H0 / L) - wy * R * theta0;

    // plw3
    vec(3) = -wz * R * theta0;

    // plw9
    vec(4) = -wz * R * theta0;

    // plw4
    vec(5) = wz * R * (L - 2 * theta0 * H0);
}