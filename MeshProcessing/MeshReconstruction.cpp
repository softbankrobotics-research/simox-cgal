#include "MeshReconstruction.h"

/*
#include <CGAL/trace.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/IO/Polyhedron_iostream.h>
#include <CGAL/make_surface_mesh.h>



#include <CGAL/IO/read_xyz_points.h>
*/


#include <CGAL/Surface_mesh_default_triangulation_3.h>
#include <CGAL/Implicit_surface_3.h>
#include <CGAL/property_map.h>
#include <CGAL/Poisson_reconstruction_function.h>
#include <CGAL/compute_average_spacing.h>
#include <CGAL/IO/output_surface_facets_to_polyhedron.h>

#include <vector>
#include <fstream>

using namespace VirtualRobot;
using namespace std;

namespace SimoxCGAL {

MeshReconstruction::MeshReconstruction(): verbose(false)
{
}


MeshReconstruction::~MeshReconstruction()
{

}

void MeshReconstruction::setVerbose(bool v)
{
    verbose = v;
}

PolyhedronMeshPtr MeshReconstruction::reconstructMesh(const std::vector<Eigen::Vector3f> &points, const std::vector<Eigen::Vector3f> &normals)
{
    if (points.size() != normals.size())
    {
        VR_ERROR << "Size of points != size of normals" << endl;
        return PolyhedronMeshPtr();
    }
    if (points.size()==0)
        return PolyhedronMeshPtr();
    if (verbose)
        VR_INFO << "Converting points to cgal data structure" << endl;

    std::vector<PointNormalPoly> pointsNormals;
    pointsNormals.reserve(points.size());
    for (size_t i=0; i<points.size(); i++)
    {
        const Eigen::Vector3f &p = points.at(i);
        const Eigen::Vector3f &n = normals.at(i);
        PointNormalPoly::Vector v(n[0], n[1], n[2]);
        PointNormalPoly pn(p[0], p[1], p[2], v);
        pointsNormals.push_back(pn);
    }

    return reconstructMesh(pointsNormals);
}


PolyhedronMeshPtr MeshReconstruction::reconstructMesh(std::vector<PointNormalPoly> &points)
{
    if (points.size()==0)
    {
        VR_ERROR << "no points to reconstruct..." << endl;
        return PolyhedronMeshPtr();
    }

    if (verbose)
        VR_INFO << "Starting to reconstruct mesh" << endl;

    // Poisson options
    KernelPolyhedron::FT sm_angle = 20.0; // Min triangle angle in degrees.
    KernelPolyhedron::FT sm_radius = 30; // Max triangle size w.r.t. point set average spacing.
    KernelPolyhedron::FT sm_distance = 0.375; // Surface Approximation error w.r.t. point set average spacing.


    if (verbose)
        VR_INFO << "Creating implicit function.." << endl;

    // Creates implicit function from the read points using the default solver.
    // Note: this method requires an iterator over points
    // + property maps to access each point's position and normal.
    // The position property map can be omitted here as we use iterators over Point_3 elements.
    CGAL::Poisson_reconstruction_function<KernelPolyhedron> function(points.begin(), points.end(),
                                             CGAL::make_normal_of_point_with_normal_pmap(std::vector<PointNormalPoly>::value_type()) );
    // Computes the Poisson indicator function f()
    // at each vertex of the triangulation.
    if ( ! function.compute_implicit_function() )
    {
        VR_ERROR << "Could not compute implicit function..." << endl;
        return PolyhedronMeshPtr();
    }
    if (verbose)
        VR_INFO << "Compute average spacing.." << endl;
    // Computes average spacing
    KernelPolyhedron::FT average_spacing = CGAL::compute_average_spacing<CGAL::Sequential_tag>(points.begin(), points.end(),
                                                       6 /* knn = 1 ring */);
    // Gets one point inside the implicit surface
    // and computes implicit function bounding sphere radius.
    PointNormalPoly inner_point = function.get_inner_point();
    KernelPolyhedron::Sphere_3 bsphere = function.bounding_sphere();
    KernelPolyhedron::FT radius = std::sqrt(bsphere.squared_radius());

    if (verbose)
        VR_INFO << "Computing surface.." << endl;

    // Defines the implicit surface: requires defining a
    // conservative bounding sphere centered at inner point.
    KernelPolyhedron::FT sm_sphere_radius = 5.0 * radius;
    KernelPolyhedron::FT sm_dichotomy_error = sm_distance*average_spacing/1000.0; // Dichotomy error must be << sm_distance
    CGAL::Implicit_surface_3< KernelPolyhedron, CGAL::Poisson_reconstruction_function<KernelPolyhedron> > surface(function,
                      KernelPolyhedron::Sphere_3(inner_point,sm_sphere_radius*sm_sphere_radius),
                      sm_dichotomy_error/sm_sphere_radius);
    // Defines surface mesh generation criteria
    CGAL::Surface_mesh_default_criteria_3<CGAL::Surface_mesh_default_triangulation_3> criteria(sm_angle,  // Min triangle angle (degrees)
                                                        sm_radius*average_spacing,  // Max triangle size
                                                        sm_distance*average_spacing); // Approximation error
    if (verbose)
        VR_INFO << "Generating mesh.." << endl;
    // Generates surface mesh with manifold option
    CGAL::Surface_mesh_default_triangulation_3 tr; // 3D Delaunay triangulation for surface mesh generation
    CGAL::Surface_mesh_complex_2_in_triangulation_3<CGAL::Surface_mesh_default_triangulation_3> c2t3(tr); // 2D complex in 3D Delaunay triangulation
    CGAL::make_surface_mesh(c2t3,                                 // reconstructed mesh
                            surface,                              // implicit surface
                            criteria,                             // meshing criteria
                            CGAL::Manifold_with_boundary_tag());  // require manifold mesh
    if(tr.number_of_vertices() == 0)
    {
        VR_ERROR << "Could not compute surface..." << endl;
        return PolyhedronMeshPtr();
    }

    if (verbose)
        VR_INFO << "Converting mesh to polyhedron..." << endl;
    // saves reconstructed surface mesh
    //std::ofstream out("kitten_poisson-20-30-0.375.off");
    PolyhedronMeshPtr result(new PolyhedronMesh());
    CGAL::output_surface_facets_to_polyhedron(c2t3, *result);
    //out << output_mesh;
    return result;
}

}
