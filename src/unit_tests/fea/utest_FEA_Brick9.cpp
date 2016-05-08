// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All right reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Antonio Recuero
// =============================================================================
// Unit tests for 9-node, large deformation brick element
// This unit test checks for internal, inertia, and gravity force correctness
// using  1) BendingQuasiStatic (static); 2) Dynamic swinging shell under gravity
// =============================================================================

#include "chrono/lcp/ChLcpIterativeMINRES.h"
#include "chrono_fea/ChElementBrick_9.h"
#include "chrono_fea/ChMesh.h"

using namespace chrono;
using namespace chrono::fea;

bool BendingQuasiStatic();
bool SwingingShell(ChMatrixDynamic<> FileInputMat);

int main(int argc, char* argv[]) {
    // Open file for reference data (Swinging shell)

    ChMatrixDynamic<> FileInputMat(41, 4);
    std::string ShellBrick9_Val_File = GetChronoDataPath() + "testing/" + "UT_SwingingShellBrick9.txt";
    std::ifstream fileMid(ShellBrick9_Val_File);
    if (!fileMid.is_open()) {
        fileMid.open(ShellBrick9_Val_File);
    }
    if (!fileMid) {
        std::cout << "Cannot open file.\n";
        exit(1);
    }
    for (int x = 0; x < 41; x++) {
        fileMid >> FileInputMat[x][0] >> FileInputMat[x][1] >> FileInputMat[x][2] >> FileInputMat[x][3];
    }
    fileMid.close();

    bool passBending = BendingQuasiStatic();
    bool passSwinging = SwingingShell(FileInputMat);
    if (passBending && passSwinging)
        return 0;
    else
        return 1;
}

// QuasiStatic
bool BendingQuasiStatic() {
    FILE* outputfile;
    ChSystem my_system;
    my_system.Set_G_acc(ChVector<>(0, 0, -9.81));

    GetLog() << "-----------------------------------------------------------\n";
    GetLog() << "-----------------------------------------------------------\n";
    GetLog() << "  9-Node, Large Deformation Brick Element: Bending Problem \n";
    GetLog() << "-----------------------------------------------------------\n";

    // Create a mesh, that is a container for groups of elements and their referenced nodes.
    auto my_mesh = std::make_shared<ChMesh>();

    // Geometry of the plate
    double plate_lenght_x = 1;
    double plate_lenght_y = 1;
    double plate_lenght_z = 0.01;
    // Specification of the mesh
    int numDiv_x = 4;
    int numDiv_y = 4;
    int numDiv_z = 1;
    int N_x = numDiv_x + 1;
    int N_y = numDiv_y + 1;
    int N_z = numDiv_z + 1;
    // Number of elements in the z direction is considered as 1
    int TotalNumElements = numDiv_x * numDiv_y;
    int XYNumNodes = (numDiv_x + 1) * (numDiv_y + 1);
    int TotalNumNodes = 2 * XYNumNodes + TotalNumElements;
    // For uniform mesh
    double dx = plate_lenght_x / numDiv_x;
    double dy = plate_lenght_y / numDiv_y;
    double dz = plate_lenght_z / numDiv_z;

    // Create and add the nodes
    for (int j = 0; j <= numDiv_z; j++) {
        for (int i = 0; i < XYNumNodes; i++) {
            // Node location
            double loc_x = (i % (numDiv_x + 1)) * dx;
            double loc_y = (i / (numDiv_x + 1)) % (numDiv_y + 1) * dy;
            double loc_z = j * dz;

            // Create the node
            auto node = std::make_shared<ChNodeFEAxyz>(ChVector<>(loc_x, loc_y, loc_z));
            node->SetMass(0);

            // Fix all nodes along the axis X=0
            if (i % (numDiv_x + 1) == 0)
                node->SetFixed(true);

            // Add node to mesh
            my_mesh->AddNode(node);
        }
    }

    for (int i = 0; i < TotalNumElements; i++) {
        auto node = std::make_shared<ChNodeFEAcurv>(ChVector<>(0.0, 0.0, 0.0), ChVector<>(0.0, 0.0, 0.0),
                                                    ChVector<>(0.0, 0.0, 0.0));
        node->SetMass(0);
        my_mesh->AddNode(node);
    }

    // Get a handle to the tip node.
    auto nodetip = std::dynamic_pointer_cast<ChNodeFEAxyz>(my_mesh->GetNode(2 * XYNumNodes - 1));

    // All layers for all elements share the same material.
    double rho = 500;
    ChVector<> E(2.1e8, 2.1e8, 2.1e8);
    ChVector<> nu(0.3, 0.3, 0.3);
    ChVector<> G(8.0769231e7, 8.0769231e7, 8.0769231e7);
    auto material = std::make_shared<ChContinuumElastic>();
    material->Set_RayleighDampingK(0.0);
    material->Set_RayleighDampingM(0.0);
    material->Set_density(rho);
    material->Set_E(E.x);
    material->Set_G(G.x);
    material->Set_v(nu.x);

    // Create the elements
    for (int i = 0; i < TotalNumElements; i++) {
        // Adjacent nodes
        int node0 = (i / (numDiv_x)) * (N_x) + i % numDiv_x;
        int node1 = (i / (numDiv_x)) * (N_x) + i % numDiv_x + 1;
        int node2 = (i / (numDiv_x)) * (N_x) + i % numDiv_x + 1 + N_x;
        int node3 = (i / (numDiv_x)) * (N_x) + i % numDiv_x + N_x;
        int node4 = (i / (numDiv_x)) * (N_x) + i % numDiv_x + XYNumNodes;
        int node5 = (i / (numDiv_x)) * (N_x) + i % numDiv_x + 1 + XYNumNodes;
        int node6 = (i / (numDiv_x)) * (N_x) + i % numDiv_x + 1 + N_x + XYNumNodes;
        int node7 = (i / (numDiv_x)) * (N_x) + i % numDiv_x + N_x + XYNumNodes;
        int node8 = 2 * XYNumNodes + i;

        // Create the element and set its nodes.
        auto element = std::make_shared<ChElementBrick_9>();
        element->SetNodes(std::dynamic_pointer_cast<ChNodeFEAxyz>(my_mesh->GetNode(node0)),
                          std::dynamic_pointer_cast<ChNodeFEAxyz>(my_mesh->GetNode(node1)),
                          std::dynamic_pointer_cast<ChNodeFEAxyz>(my_mesh->GetNode(node2)),
                          std::dynamic_pointer_cast<ChNodeFEAxyz>(my_mesh->GetNode(node3)),
                          std::dynamic_pointer_cast<ChNodeFEAxyz>(my_mesh->GetNode(node4)),
                          std::dynamic_pointer_cast<ChNodeFEAxyz>(my_mesh->GetNode(node5)),
                          std::dynamic_pointer_cast<ChNodeFEAxyz>(my_mesh->GetNode(node6)),
                          std::dynamic_pointer_cast<ChNodeFEAxyz>(my_mesh->GetNode(node7)),
                          std::dynamic_pointer_cast<ChNodeFEAcurv>(my_mesh->GetNode(node8)));

        // Set element dimensions
        element->SetDimensions(ChVector<>(dx, dy, dz));

        // Add a single layers with a fiber angle of 0 degrees.
        element->SetMaterial(material);

        // Set other element properties
        element->SetAlphaDamp(0.0);    // Structural damping for this element
        element->SetGravityOn(false);  // Turn internal gravitational force calculation off

        element->SetHenckyStrain(false);
        element->SetPlasticity(false);

        // Add element to mesh
        my_mesh->AddElement(element);
    }

    // Add the mesh to the system
    my_system.Add(my_mesh);

    my_system.Set_G_acc(ChVector<>(0.0, 0.0, 0.0));

    // Mark completion of system construction
    my_system.SetupInitial();

    // Set up solver
    my_system.SetLcpSolverType(ChSystem::LCP_ITERATIVE_MINRES);
    chrono::ChLcpIterativeMINRES* msolver = (chrono::ChLcpIterativeMINRES*)my_system.GetLcpSolverSpeed();
    msolver->SetDiagonalPreconditioning(true);
    my_system.SetIterLCPmaxItersSpeed(300);
    my_system.SetTolForce(1e-09);

    my_system.Setup();
    my_system.Update();

    double force1 = -50;
    nodetip->SetForce(ChVector<>(0.0, 0.0, force1));

    my_system.DoStaticNonlinear(1000);
    GetLog() << "Final value: " << nodetip->GetPos().z << "\n";

    // Reference vertical position
    // double refZTip = -0.437682; // For GL strain

    double refZTip = -0.4375;
    bool pass = false;
    if (abs(refZTip - nodetip->GetPos().z) < abs(refZTip) / 100)
        pass = true;

    return pass;
}

// Swinging (Bricked) Shell
bool SwingingShell(ChMatrixDynamic<> FileInputMat) {
    FILE* outputfile;
    double precision = 1e-3;  // Precision for test

    bool genRefFile = false;
    ChSystem my_system;
    my_system.Set_G_acc(ChVector<>(0, 0, 0));

    GetLog() << "--------------------------------------------------------------------\n";
    GetLog() << "--------------------------------------------------------------------\n";
    GetLog() << " 9-Node, Large Deformation Brick Element: Swinging (Bricked) Shell  \n";
    GetLog() << "--------------------------------------------------------------------\n";

    // Create a mesh, that is a container for groups of elements and their referenced nodes.
    auto my_mesh = std::make_shared<ChMesh>();

    // Geometry of the plate
    double plate_lenght_x = 1;
    double plate_lenght_y = 1;
    double plate_lenght_z = 0.01;

    // Specification of the mesh
    int numDiv_x = 8;
    int numDiv_y = 8;
    int numDiv_z = 1;
    int N_x = numDiv_x + 1;
    int N_y = numDiv_y + 1;
    int N_z = numDiv_z + 1;

    // Number of elements in the z direction is considered as 1
    int TotalNumElements = numDiv_x * numDiv_y;
    int XYNumNodes = (numDiv_x + 1) * (numDiv_y + 1);
    int TotalNumNodes = 2 * XYNumNodes + TotalNumElements;

    // For uniform mesh
    double dx = plate_lenght_x / numDiv_x;
    double dy = plate_lenght_y / numDiv_y;
    double dz = plate_lenght_z / numDiv_z;
    double timestep = 5e-3;

    // Create and add the nodes
    for (int j = 0; j <= numDiv_z; j++) {
        for (int i = 0; i < XYNumNodes; i++) {
            // Node location
            double loc_x = (i % (numDiv_x + 1)) * dx;
            double loc_y = (i / (numDiv_x + 1)) % (numDiv_y + 1) * dy;
            double loc_z = j * dz;

            // Create the node
            auto node = std::make_shared<ChNodeFEAxyz>(ChVector<>(loc_x, loc_y, loc_z));
            node->SetMass(0);
            // Fix all nodes along the axis X=0
            if (i == 0 && j == 0)
                node->SetFixed(true);

            // Add node to mesh
            my_mesh->AddNode(node);
        }
    }

    for (int i = 0; i < TotalNumElements; i++) {
        auto node = std::make_shared<ChNodeFEAcurv>(ChVector<>(0.0, 0.0, 0.0), ChVector<>(0.0, 0.0, 0.0),
                                                    ChVector<>(0.0, 0.0, 0.0));
        node->SetMass(0);
        my_mesh->AddNode(node);
    }

    double force = 0.0;
    // Get a handle to the tip node.
    auto nodetip = std::dynamic_pointer_cast<ChNodeFEAxyz>(my_mesh->GetNode(2 * XYNumNodes - 1));
    nodetip->SetForce(ChVector<>(0.0, 0.0, -0.0));

    // All layers for all elements share the same material.
    double rho = 1000;
    ChVector<> E(2.1e7, 2.1e7, 2.1e7);
    ChVector<> nu(0.3, 0.3, 0.3);
    ChVector<> G(8.0769231e6, 8.0769231e6, 8.0769231e6);
    auto material = std::make_shared<ChContinuumElastic>();
    material->Set_RayleighDampingK(0.0);
    material->Set_RayleighDampingM(0.0);
    material->Set_density(rho);
    material->Set_E(E.x);
    material->Set_G(G.x);
    material->Set_v(nu.x);

    // Create the elements
    for (int i = 0; i < TotalNumElements; i++) {
        // Adjacent nodes
        int node0 = (i / (numDiv_x)) * (N_x) + i % numDiv_x;
        int node1 = (i / (numDiv_x)) * (N_x) + i % numDiv_x + 1;
        int node2 = (i / (numDiv_x)) * (N_x) + i % numDiv_x + 1 + N_x;
        int node3 = (i / (numDiv_x)) * (N_x) + i % numDiv_x + N_x;
        int node4 = (i / (numDiv_x)) * (N_x) + i % numDiv_x + XYNumNodes;
        int node5 = (i / (numDiv_x)) * (N_x) + i % numDiv_x + 1 + XYNumNodes;
        int node6 = (i / (numDiv_x)) * (N_x) + i % numDiv_x + 1 + N_x + XYNumNodes;
        int node7 = (i / (numDiv_x)) * (N_x) + i % numDiv_x + N_x + XYNumNodes;
        int node8 = 2 * XYNumNodes + i;

        // Create the element and set its nodes.
        auto element = std::make_shared<ChElementBrick_9>();
        element->SetNodes(std::dynamic_pointer_cast<ChNodeFEAxyz>(my_mesh->GetNode(node0)),
                          std::dynamic_pointer_cast<ChNodeFEAxyz>(my_mesh->GetNode(node1)),
                          std::dynamic_pointer_cast<ChNodeFEAxyz>(my_mesh->GetNode(node2)),
                          std::dynamic_pointer_cast<ChNodeFEAxyz>(my_mesh->GetNode(node3)),
                          std::dynamic_pointer_cast<ChNodeFEAxyz>(my_mesh->GetNode(node4)),
                          std::dynamic_pointer_cast<ChNodeFEAxyz>(my_mesh->GetNode(node5)),
                          std::dynamic_pointer_cast<ChNodeFEAxyz>(my_mesh->GetNode(node6)),
                          std::dynamic_pointer_cast<ChNodeFEAxyz>(my_mesh->GetNode(node7)),
                          std::dynamic_pointer_cast<ChNodeFEAcurv>(my_mesh->GetNode(node8)));

        // Set element dimensions
        element->SetDimensions(ChVector<>(dx, dy, dz));

        // Add a single layers with a fiber angle of 0 degrees.
        element->SetMaterial(material);

        // Set other element properties
        element->SetAlphaDamp(0.005);  // Structural damping for this element
        element->SetGravityOn(true);   // turn internal gravitational force calculation off

        element->SetHenckyStrain(true);
        element->SetPlasticity(false);

        // Add element to mesh
        my_mesh->AddElement(element);
    }

    // Add the mesh to the system
    my_system.Add(my_mesh);

    // -------------------------------------
    // Options for visualization in irrlicht
    // -------------------------------------

    my_system.Set_G_acc(ChVector<>(0.0, 0.0, -9.81));
    my_mesh->SetAutomaticGravity(false);

    // Mark completion of system construction
    my_system.SetupInitial();

    // ----------------------------------
    // Perform a dynamic time integration
    // ----------------------------------

    // Set up solver
    my_system.SetLcpSolverType(ChSystem::LCP_ITERATIVE_MINRES);  // <- NEEDED because other solvers can't
                                                                 // handle stiffness matrices
    chrono::ChLcpIterativeMINRES* msolver = (chrono::ChLcpIterativeMINRES*)my_system.GetLcpSolverSpeed();
    msolver->SetDiagonalPreconditioning(true);
    my_system.SetIterLCPmaxItersSpeed(500);
    my_system.SetTolForce(1e-08);

    // Set the time integrator parameters
    my_system.SetIntegrationType(ChSystem::INT_HHT);
    auto mystepper = std::dynamic_pointer_cast<ChTimestepperHHT>(my_system.GetTimestepper());
    mystepper->SetAlpha(-0.2);
    mystepper->SetMaxiters(20);
    mystepper->SetAbsTolerances(1e-6, 1e-1);
    mystepper->SetMode(ChTimestepperHHT::POSITION);
    mystepper->SetVerbose(false);
    mystepper->SetScaling(true);

    my_system.Setup();
    my_system.Update();

    if (genRefFile) {
        outputfile = fopen("SwingingShellRef.txt", "w");
        fprintf(outputfile, "%15.7e  ", my_system.GetChTime());
        fprintf(outputfile, "%15.7e  ", nodetip->GetPos().x);
        fprintf(outputfile, "%15.7e  ", nodetip->GetPos().y);
        fprintf(outputfile, "%15.7e  ", nodetip->GetPos().z);
        fprintf(outputfile, "\n  ");
    }
    unsigned int it = 0;
    double RelVal, RelVal1, RelVal2, RelVal3;

    while (my_system.GetChTime() < 0.05) {
        my_system.DoStepDynamics(timestep);
        it++;

        if (genRefFile) {
            fprintf(outputfile, "%15.7e  ", my_system.GetChTime());
            fprintf(outputfile, "%15.7e  ", nodetip->GetPos().x);
            fprintf(outputfile, "%15.7e  ", nodetip->GetPos().y);
            fprintf(outputfile, "%15.7e  ", nodetip->GetPos().z);
            fprintf(outputfile, "\n  ");
        } else {
            RelVal1 = std::abs(nodetip->pos.x - FileInputMat[it][1]) / std::abs(FileInputMat[it][1]);
            RelVal2 = std::abs(nodetip->pos.y - FileInputMat[it][2]) / std::abs(FileInputMat[it][2]);
            RelVal3 = std::abs(nodetip->pos.z - FileInputMat[it][3]) / std::abs(FileInputMat[it][3]);
            RelVal = RelVal1 + RelVal2 + RelVal3;
            // GetLog() << RelVal << "\n";
            if (RelVal > precision) {
                std::cout << "Unit test check failed \n";
                return false;
            }
        }
    }
    return true;
}
