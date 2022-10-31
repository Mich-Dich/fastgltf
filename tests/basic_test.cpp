#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "fastgltf_parser.hpp"
#include "fastgltf_types.hpp"

// We need to use the __FILE__ macro so that we have access to test glTF files in this
// directory. As Clang does not yet fully support std::source_location, we cannot use that.
auto path = std::filesystem::path { __FILE__ }.parent_path() / "gltf";

TEST_CASE("Component type tests", "[gltf-loader]") {
    using namespace fastgltf;

    // clang-format off
    REQUIRE(fastgltf::getNumComponents(AccessorType::Scalar) ==  1);
    REQUIRE(fastgltf::getNumComponents(AccessorType::Vec2)   ==  2);
    REQUIRE(fastgltf::getNumComponents(AccessorType::Vec3)   ==  3);
    REQUIRE(fastgltf::getNumComponents(AccessorType::Vec4)   ==  4);
    REQUIRE(fastgltf::getNumComponents(AccessorType::Mat2)   ==  4);
    REQUIRE(fastgltf::getNumComponents(AccessorType::Mat3)   ==  9);
    REQUIRE(fastgltf::getNumComponents(AccessorType::Mat4)   == 16);

    REQUIRE(fastgltf::getComponentBitSize(ComponentType::Byte)          ==  8);
    REQUIRE(fastgltf::getComponentBitSize(ComponentType::UnsignedByte)  ==  8);
    REQUIRE(fastgltf::getComponentBitSize(ComponentType::Short)         == 16);
    REQUIRE(fastgltf::getComponentBitSize(ComponentType::UnsignedShort) == 16);
    REQUIRE(fastgltf::getComponentBitSize(ComponentType::UnsignedInt)   == 32);
    REQUIRE(fastgltf::getComponentBitSize(ComponentType::Float)         == 32);
    REQUIRE(fastgltf::getComponentBitSize(ComponentType::Double)        == 64);
    REQUIRE(fastgltf::getComponentBitSize(ComponentType::Invalid)       ==  0);

    REQUIRE(fastgltf::getElementByteSize(AccessorType::Scalar, ComponentType::Byte)  == 1);
    REQUIRE(fastgltf::getElementByteSize(AccessorType::Vec4,   ComponentType::Byte)  == 4);
    REQUIRE(fastgltf::getElementByteSize(AccessorType::Vec4,   ComponentType::Short) == 8);

    REQUIRE(fastgltf::getComponentType(5120) == ComponentType::Byte);
    REQUIRE(fastgltf::getComponentType(5121) == ComponentType::UnsignedByte);
    REQUIRE(fastgltf::getComponentType(5122) == ComponentType::Short);
    REQUIRE(fastgltf::getComponentType(5123) == ComponentType::UnsignedShort);
    REQUIRE(fastgltf::getComponentType(5125) == ComponentType::UnsignedInt);
    REQUIRE(fastgltf::getComponentType(5126) == ComponentType::Float);
    REQUIRE(fastgltf::getComponentType(5130) == ComponentType::Double);
    REQUIRE(fastgltf::getComponentType(5131) == ComponentType::Invalid);
    // clang-format on
};

TEST_CASE("Loading some basic glTF", "[gltf-loader]") {
    fastgltf::Parser parser;
    SECTION("Loading basic invalid glTF files") {
        auto jsonData = std::make_unique<fastgltf::JsonData>(path / "empty_json.gltf");
        auto emptyGltf = parser.loadGLTF(jsonData.get(), path);
        REQUIRE(emptyGltf->parse() == fastgltf::Error::InvalidOrMissingAssetField);
    }

    SECTION("Load basic glTF file") {
        auto basicJsonData = std::make_unique<fastgltf::JsonData>(path / "basic_gltf.gltf");
        auto basicGltf = parser.loadGLTF(basicJsonData.get(), path);
        REQUIRE(basicGltf != nullptr);
        REQUIRE(parser.getError() == fastgltf::Error::None);
    }

    SECTION("Loading basic Cube.gltf") {
        auto cubePath = path / "sample-models" / "2.0" / "Cube" / "glTF";
        auto cubeJsonData = std::make_unique<fastgltf::JsonData>(cubePath / "Cube.gltf");
        auto cubeGltf = parser.loadGLTF(cubeJsonData.get(), cubePath);
        REQUIRE(parser.getError() == fastgltf::Error::None);
        REQUIRE(cubeGltf != nullptr);

        REQUIRE(cubeGltf->parse(fastgltf::Category::Scenes) == fastgltf::Error::None);

        auto cube = cubeGltf->getParsedAsset();
        REQUIRE(cube->scenes.size() == 1);
        REQUIRE(cube->scenes.front().nodeIndices.size() == 1);
        REQUIRE(cube->scenes.front().nodeIndices.front() == 0);

        REQUIRE(cube->nodes.size() == 1);
        REQUIRE(cube->nodes.front().name == "Cube");
        REQUIRE(!cube->nodes.front().hasMatrix);

        REQUIRE(cube->accessors.size() == 5);
        REQUIRE(cube->accessors[0].type == fastgltf::AccessorType::Scalar);
        REQUIRE(cube->accessors[0].componentType == fastgltf::ComponentType::UnsignedShort);
        REQUIRE(cube->accessors[1].type == fastgltf::AccessorType::Vec3);
        REQUIRE(cube->accessors[1].componentType == fastgltf::ComponentType::Float);

        REQUIRE(cube->bufferViews.size() == 5);
        REQUIRE(cube->buffers.size() == 1);

        REQUIRE(cube->materials.size() == 1);
        auto& material = cube->materials.front();
        REQUIRE(material.name == "Cube");
        REQUIRE(material.pbrData.has_value());
        REQUIRE(material.pbrData->baseColorTexture.has_value());
        REQUIRE(material.pbrData->baseColorTexture->textureIndex == 0);
        REQUIRE(material.pbrData->metallicRoughnessTexture.has_value());
        REQUIRE(material.pbrData->metallicRoughnessTexture->textureIndex == 1);
        REQUIRE(!material.normalTexture.has_value());
        REQUIRE(!material.emissiveTexture.has_value());
        REQUIRE(!material.occlusionTexture.has_value());
    }

    SECTION("Loading basic Box.gltf") {
        auto boxPath = path / "sample-models" / "2.0" / "Box" / "glTF";
        auto boxJsonData = std::make_unique<fastgltf::JsonData>(boxPath / "Box.gltf");
        auto boxGltf = parser.loadGLTF(boxJsonData.get(), boxPath);
        REQUIRE(parser.getError() == fastgltf::Error::None);
        REQUIRE(boxGltf != nullptr);

        REQUIRE(boxGltf->parse(fastgltf::Category::Scenes) == fastgltf::Error::None);

        auto box = boxGltf->getParsedAsset();
        REQUIRE(box->defaultScene.has_value());
        REQUIRE(box->defaultScene.value() == 0);

        REQUIRE(box->nodes.size() == 2);
        REQUIRE(box->nodes[0].children.size() == 1);
        REQUIRE(box->nodes[0].children[0] == 1);
        REQUIRE(box->nodes[1].children.empty());
        REQUIRE(box->nodes[1].meshIndex.has_value());
        REQUIRE(box->nodes[1].meshIndex.value() == 0);

        REQUIRE(box->materials.size() == 1);
        REQUIRE(box->materials[0].name == "Red");
        REQUIRE(box->materials[0].pbrData.has_value());
        REQUIRE(box->materials[0].pbrData->baseColorFactor[3] == 1.0f);
        REQUIRE(box->materials[0].pbrData->metallicFactor == 0.0f);
    }
};

TEST_CASE("Loading KHR_texture_basisu glTF files", "[gltf-loader]") {
    auto stainedLamp = path / "sample-models" / "2.0" / "StainedGlassLamp" / "glTF-KTX-BasisU";

    auto jsonData = std::make_unique<fastgltf::JsonData>(stainedLamp / "StainedGlassLamp.gltf");

    SECTION("Loading KHR_texture_basisu") {
        fastgltf::Parser parser(fastgltf::Extensions::KHR_texture_basisu);
        auto stainedGlassLamp = parser.loadGLTF(jsonData.get(), path, fastgltf::Options::DontRequireValidAssetMember);
        REQUIRE(parser.getError() == fastgltf::Error::None);
        REQUIRE(stainedGlassLamp != nullptr);

        REQUIRE(stainedGlassLamp->parse(fastgltf::Category::Textures) == fastgltf::Error::None);

        auto asset = stainedGlassLamp->getParsedAsset();
        REQUIRE(asset->textures.size() == 19);
        REQUIRE(!asset->images.empty());

        auto& texture = asset->textures[1];
        REQUIRE(texture.imageIndex == 1);
        REQUIRE(texture.samplerIndex == 0);
        REQUIRE(!texture.fallbackImageIndex.has_value());

        auto& image = asset->images.front();
        REQUIRE(image.location == fastgltf::DataLocation::FilePathWithByteRange);
        REQUIRE(image.data.mimeType == fastgltf::MimeType::KTX2);
    }

    SECTION("Testing requiredExtensions") {
        // We specify no extensions, yet the StainedGlassLamp requires KHR_texture_basisu.
        fastgltf::Parser parser(fastgltf::Extensions::None);
        auto stainedGlassLamp = parser.loadGLTF(jsonData.get(), path, fastgltf::Options::DontRequireValidAssetMember);
        REQUIRE(stainedGlassLamp->parse() == fastgltf::Error::MissingExtensions);
    }
};

TEST_CASE("Loading KHR_texture_transform glTF files", "[gltf-loader]") {
    auto transformTest = path / "sample-models" / "2.0" / "TextureTransformMultiTest" / "glTF";

    auto jsonData = std::make_unique<fastgltf::JsonData>(transformTest / "TextureTransformMultiTest.gltf");

    fastgltf::Parser parser(fastgltf::Extensions::KHR_texture_transform);
    auto test = parser.loadGLTF(jsonData.get(), transformTest, fastgltf::Options::DontRequireValidAssetMember);
    REQUIRE(parser.getError() == fastgltf::Error::None);
    REQUIRE(test != nullptr);

    REQUIRE(test->parse(fastgltf::Category::Materials) == fastgltf::Error::None);

    auto asset = test->getParsedAsset();
    REQUIRE(!asset->materials.empty());

    auto& material = asset->materials.front();
    REQUIRE(material.pbrData.has_value());
    REQUIRE(material.pbrData->baseColorTexture.has_value());
    REQUIRE(material.pbrData->baseColorTexture->uvOffset[0] == 0.705f);
    REQUIRE(material.pbrData->baseColorTexture->rotation == Catch::Approx(1.5707963705062866f));
}

TEST_CASE("Loading glTF animation", "[gltf-loader]") {
    auto animatedCube = path / "sample-models" / "2.0" / "AnimatedCube" / "glTF";

    auto jsonData = std::make_unique<fastgltf::JsonData>(animatedCube / "AnimatedCube.gltf");

    fastgltf::Parser parser;
    auto cube = parser.loadGLTF(jsonData.get(), animatedCube);
    REQUIRE(parser.getError() == fastgltf::Error::None);
    REQUIRE(cube != nullptr);

    REQUIRE(cube->parse(fastgltf::Category::Animations) == fastgltf::Error::None);

    auto asset = cube->getParsedAsset();
    REQUIRE(!asset->animations.empty());

    auto& animation = asset->animations.front();
    REQUIRE(animation.name == "animation_AnimatedCube");

    REQUIRE(!animation.channels.empty());
    REQUIRE(animation.channels.front().nodeIndex == 0);
    REQUIRE(animation.channels.front().samplerIndex == 0);
    REQUIRE(animation.channels.front().path == fastgltf::AnimationPath::Rotation);

    REQUIRE(!animation.samplers.empty());
    REQUIRE(animation.samplers.front().interpolation == fastgltf::AnimationInterpolation::Linear);
    REQUIRE(animation.samplers.front().inputAccessor == 0);
    REQUIRE(animation.samplers.front().outputAccessor == 1);
}

TEST_CASE("Loading glTF skins", "[gltf-loader]") {
    auto simpleSkin = path / "sample-models" / "2.0" / "SimpleSkin" / "glTF";

    auto jsonData = std::make_unique<fastgltf::JsonData>(simpleSkin / "SimpleSkin.gltf");

    fastgltf::Parser parser;
    auto model = parser.loadGLTF(jsonData.get(), simpleSkin);
    REQUIRE(parser.getError() == fastgltf::Error::None);
    REQUIRE(model != nullptr);

    REQUIRE(model->parse(fastgltf::Category::Nodes) == fastgltf::Error::None);

    auto asset = model->getParsedAsset();
    REQUIRE(!asset->skins.empty());

    auto& skin = asset->skins.front();
    REQUIRE(skin.joints.size() == 2);
    REQUIRE(skin.joints[0] == 1);
    REQUIRE(skin.joints[1] == 2);
    REQUIRE(skin.inverseBindMatrices.has_value());
    REQUIRE(skin.inverseBindMatrices.value() == 4);

    REQUIRE(!asset->nodes.empty());

    auto& node = asset->nodes.front();
    REQUIRE(node.skinIndex.has_value());
    REQUIRE(node.skinIndex == 0);
}

TEST_CASE("Loading glTF cameras", "[gltf-loader]") {
    auto cameras = path / "sample-models" / "2.0" / "Cameras" / "glTF";
    auto jsonData = std::make_unique<fastgltf::JsonData>(cameras / "Cameras.gltf");

    fastgltf::Parser parser;
    auto model = parser.loadGLTF(jsonData.get(), cameras);
    REQUIRE(parser.getError() == fastgltf::Error::None);
    REQUIRE(model != nullptr);

    REQUIRE(model->parse(fastgltf::Category::Cameras) == fastgltf::Error::None);

    auto asset = model->getParsedAsset();
    REQUIRE(asset->cameras.size() == 2);

    REQUIRE(asset->cameras[0].type == fastgltf::CameraType::Perspective);
    REQUIRE(asset->cameras[1].type == fastgltf::CameraType::Orthographic);

    REQUIRE(asset->cameras[0].camera.perspective.aspectRatio == 1.0f);
    REQUIRE(asset->cameras[0].camera.perspective.yfov == 0.7f);
    REQUIRE(asset->cameras[0].camera.perspective.zfar == 100);
    REQUIRE(asset->cameras[0].camera.perspective.znear == 0.01f);

    REQUIRE(asset->cameras[1].camera.orthographic.xmag == 1.0f);
    REQUIRE(asset->cameras[1].camera.orthographic.ymag == 1.0f);
    REQUIRE(asset->cameras[1].camera.orthographic.zfar == 100);
    REQUIRE(asset->cameras[1].camera.orthographic.znear == 0.01f);
}

TEST_CASE("Validate whole glTF", "[gltf-loader]") {
    auto sponza = path / "sample-models" / "2.0" / "Sponza" / "glTF";
    auto jsonData = std::make_unique<fastgltf::JsonData>(sponza / "Sponza.gltf");

    fastgltf::Parser parser;
    auto model = parser.loadGLTF(jsonData.get(), sponza);
    REQUIRE(parser.getError() == fastgltf::Error::None);
    REQUIRE(model != nullptr);

    REQUIRE(model->parse() == fastgltf::Error::None);
    REQUIRE(model->validate() == fastgltf::Error::None);

    auto brainStem = path / "sample-models" / "2.0" / "BrainStem" / "glTF";
    jsonData = std::make_unique<fastgltf::JsonData>(brainStem / "BrainStem.gltf");

    model = parser.loadGLTF(jsonData.get(), brainStem);
    REQUIRE(parser.getError() == fastgltf::Error::None);
    REQUIRE(model != nullptr);

    REQUIRE(model->parse() == fastgltf::Error::None);
    REQUIRE(model->validate() == fastgltf::Error::None);
}

TEST_CASE("Test allocation callbacks for embedded buffers", "[gltf-loader]") {
    auto boxPath = path / "sample-models" / "2.0" / "Box" / "glTF-Embedded";
    auto jsonData = std::make_unique<fastgltf::JsonData>(boxPath / "Box.gltf");

    std::vector<void*> allocations;

    auto mapCallback = [](uint64_t bufferSize, void* userPointer) -> fastgltf::BufferInfo {
        REQUIRE(userPointer != nullptr);
        auto* allocations = static_cast<std::vector<void*>*>(userPointer);
        allocations->emplace_back(std::malloc(bufferSize));
        return {
            .mappedMemory = allocations->back(),
            .customId = allocations->size() - 1,
        };
    };

    fastgltf::Parser parser;
    parser.setUserPointer(&allocations);
    parser.setBufferAllocationCallback(mapCallback, nullptr);
    auto model = parser.loadGLTF(jsonData.get(), boxPath);
    REQUIRE(model->parse(fastgltf::Category::Buffers) == fastgltf::Error::None);
    REQUIRE(allocations.size() == 1);

    auto asset = model->getParsedAsset();
    REQUIRE(asset->buffers.size() == 1);
    REQUIRE(asset->buffers.front().location == fastgltf::DataLocation::CustomBufferWithId);
    REQUIRE(asset->buffers.front().data.bufferId == 0);

    for (auto& allocation : allocations) {
        REQUIRE(allocation != nullptr);
        std::free(allocation);
    }
}

TEST_CASE("Test TRS parsing and optional decomposition", "[gltf-loader]") {
    SECTION("Test decomposition on glTF asset") {
        auto jsonData = std::make_unique<fastgltf::JsonData>(path / "transform_matrices.gltf");

        // Parse once without decomposing, once with decomposing the matrix.
        fastgltf::Parser parser;
        auto modelWithMatrix = parser.loadGLTF(jsonData.get(), path);
        REQUIRE(parser.getError() == fastgltf::Error::None);
        REQUIRE(modelWithMatrix != nullptr);

        REQUIRE(modelWithMatrix->parse(fastgltf::Category::Nodes) == fastgltf::Error::None);
        auto assetWithMatrix = modelWithMatrix->getParsedAsset();

        auto modelDecomposed = parser.loadGLTF(jsonData.get(), path, fastgltf::Options::DecomposeNodeMatrices);
        REQUIRE(parser.getError() == fastgltf::Error::None);
        REQUIRE(modelWithMatrix != nullptr);

        REQUIRE(modelDecomposed->parse(fastgltf::Category::Nodes) == fastgltf::Error::None);
        auto assetDecomposed = modelDecomposed->getParsedAsset();

        REQUIRE(assetWithMatrix->cameras.size() == 1);
        REQUIRE(assetDecomposed->cameras.size() == 1);
        REQUIRE(assetWithMatrix->nodes.size() == 2);
        REQUIRE(assetDecomposed->nodes.size() == 2);
        REQUIRE(assetWithMatrix->nodes.back().hasMatrix);
        REQUIRE(!assetDecomposed->nodes.back().hasMatrix);

        // Get the TRS components from the first node and use them as the test data for decomposing.
        auto translation = glm::make_vec3(assetWithMatrix->nodes.front().transform.trs.translation.data());
        auto rotation = glm::make_quat(assetWithMatrix->nodes.front().transform.trs.rotation.data());
        auto scale = glm::make_vec3(assetWithMatrix->nodes.front().transform.trs.scale.data());
        auto rotationMatrix = glm::toMat4(rotation);
        auto transform = glm::translate(glm::mat4(1.0f), translation) * rotationMatrix * glm::scale(glm::mat4(1.0f), scale);

        // Check if the parsed matrix is correct.
        REQUIRE(glm::make_mat4x4(assetWithMatrix->nodes.back().transform.matrix.data()) == transform);

        // Check if the decomposed components equal the original components.
        REQUIRE(glm::make_vec3(assetDecomposed->nodes.back().transform.trs.translation.data()) == translation);
        REQUIRE(glm::make_quat(assetDecomposed->nodes.back().transform.trs.rotation.data()) == rotation);
        REQUIRE(glm::make_vec3(assetDecomposed->nodes.back().transform.trs.scale.data()) == scale);
    }

    SECTION("Test decomposition against glm decomposition") {
        // Some random complex transform matrix from one of the glTF sample models.
        std::array<float, 16> matrix = {
            -0.4234085381031037,
            -0.9059388637542724,
            -7.575183536001616e-11,
            0.0,
            -0.9059388637542724,
            0.4234085381031037,
            -4.821281221478735e-11,
            0.0,
            7.575183536001616e-11,
            4.821281221478735e-11,
            -1.0,
            0.0,
            -90.59386444091796,
            -24.379817962646489,
            -40.05522918701172,
            1.0
        };

        std::array<float, 3> translation = {}, scale = {};
        std::array<float, 4> rotation = {};
        fastgltf::decomposeTransformMatrix(matrix, scale, rotation, translation);

        auto glmMatrix = glm::make_mat4x4(matrix.data());
        glm::vec3 glmScale, glmTranslation, glmSkew;
        glm::quat glmRotation;
        glm::vec4 glmPerspective;
        glm::decompose(glmMatrix, glmScale, glmRotation, glmTranslation, glmSkew, glmPerspective);

        // I use glm::epsilon<float>() * 10 here because some matrices I tested this with resulted
        // in an error margin greater than the normal epsilon value. I will investigate this in the
        // future, but I suspect using double in the decompose functions should help mitigate most
        // of it.
        REQUIRE(glm::make_vec3(translation.data()) == glmTranslation);
        REQUIRE(glm::all(glm::epsilonEqual(glm::make_quat(rotation.data()), glmRotation, glm::epsilon<float>() * 10)));
        REQUIRE(glm::all(glm::epsilonEqual(glm::make_vec3(scale.data()), glmScale, glm::epsilon<float>())));
    }
}
