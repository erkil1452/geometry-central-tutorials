#include "stdafx.h"

using namespace geometrycentral;
using namespace geometrycentral::surface;
namespace fs = std::filesystem;


//std::vector<glm::vec3> direction2ToDir3(VertexData<Vector2> field)

inline float sanitized_float(const float x) { return isfinite(x) ? x : 0; }

int main(int argc, char** argv) {

  // Configure a simple argument parser
  // clang-format off
  args::ArgumentParser parser("");
  args::HelpFlag help(parser, "help", "Display this help message", {'h', "help"});

  args::Positional<std::string> inputFilename(parser, "mesh", "A surface mesh file (see geometry-central docs for valid formats)");
  args::Group group(parser, "This group is all exclusive:", args::Group::Validators::DontCare);
  args::Flag silent(group, "silent", "Silent mode", {'s', "silent"});
  // clang-format on

  // Parse args
  try {
    parser.ParseCLI(argc, argv);
  } catch (args::Help) {
    std::cout << parser;
    return 0;
  } catch (args::ParseError e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  }

  // Make sure a mesh name was given
  if (!inputFilename) {
    std::cout << parser;
    return EXIT_FAILURE;
  }

  // Load a general surface mesh from file
  std::unique_ptr<SurfaceMesh> mesh;
  std::unique_ptr<VertexPositionGeometry> geometry;
  std::tie(mesh, geometry) = readManifoldSurfaceMesh(args::get(inputFilename));
  //std::tie(mesh, geometry) = readSurfaceMesh(args::get(inputFilename));

  polyscope::init();
  auto* psMesh =
      polyscope::registerSurfaceMesh("input mesh",
          geometry->inputVertexPositions, 
          mesh->getFaceVertexList());

  // Tell polyscope about vertex tangent spaces
  geometry->requireVertexTangentBasis();
  VertexData<Vector3> vBasisX(*mesh);
  for (Vertex v : mesh->vertices()) {
    vBasisX[v] = geometry->vertexTangentBasis[v][0];
  }
  psMesh->setVertexTangentBasisX(vBasisX);


  VertexData<Vector2> field = computeSmoothestVertexDirectionField(*geometry);
  auto direction_quantity = dynamic_cast<polyscope::SurfaceVertexIntrinsicVectorQuantity*>(
      psMesh->addVertexIntrinsicVectorQuantity("vectors", field));
  direction_quantity->setEnabled(true);
  
  // Write out.
  auto inputPath = fs::path(args::get(inputFilename));
  auto outputPath = inputPath.parent_path() / (inputPath.stem().string() + "_vertex_dir_field.txt");
  //writeSurfaceMesh(*mesh, *geometry, outputPath.string());
  auto verts = direction_quantity->vectorRoots;
  auto normals = psMesh->vertexNormals;
  auto tangents = direction_quantity->vectors;
  std::ofstream s(outputPath);
  if (!s.is_open()) {
    std::cout << "failed to open " << outputPath << '\n';
  } else {
    s << "Vertex X Y Z, Normal X Y Z, Tangent X Y Z\n";
    for (auto i = 0; i < field.size(); i++) {
      s << sanitized_float(verts[i].x) << " " << sanitized_float(verts[i].y) << " " << sanitized_float(verts[i].z) << " "; // pos
      s << sanitized_float(normals[i].x) << " " << sanitized_float(normals[i].y) << " " << sanitized_float(normals[i].z) << " "; // normal
      s << sanitized_float(tangents[i].x) << " " << sanitized_float(tangents[i].y) << " " << sanitized_float(tangents[i].z) << " "; // tangent
      s << '\n';
    }
  }
  s.close();

  if (!silent) {
    polyscope::show();
  }

  std::cout << "DONE." << std::endl;

  return EXIT_SUCCESS;
}
