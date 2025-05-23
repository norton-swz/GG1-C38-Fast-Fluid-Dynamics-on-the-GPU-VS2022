/**
 * @file helper.h
 * @author Eron Ristich (eron@ristich.com)
 * @brief Details a set of helpful struct objects that can be used to describe various properties of objects. Currently includes structs for vertices, textures, and classes for vectors, shaders, meshes (which can be polymorphed as necessary), and models (collection of meshes, implements Assimp model loading)
 * @version 0.1
 * @date 2022-05-31
 * 
 * @copyright Copyright (c) 2022
 */

#ifndef HELPER_H
#define HELPER_H

#include <SDL_image.h>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/glu.h>
#include <GL/gl.h>
#include <GL/glut.h>

#include <vector>
using std::vector;
#include <string>
using std::string;

#include <iostream>
#include <fstream>
#include <sstream>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/matrix.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define MAX_BONE_INFLUENCE 4

inline unsigned int textureFromFile(const char *path, const string &directory, bool gamma = false);

/**
 * @brief Defines a single vertex in OpenGL space (adapted from https://learnopengl.com/Model-Loading/Mesh)
 */
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec3 tangent;
    glm::vec3 bitangent;
	int m_BoneIDs[MAX_BONE_INFLUENCE];
	float m_Weights[MAX_BONE_INFLUENCE];
};

/**
 * @brief Defines a single point's worth of texture data in OpenGL space (adapted from https://learnopengl.com/Model-Loading/Mesh)
 */
struct Texture {
    unsigned int id;
    string type;
    string path;
};

/**
 * @brief Abstract light class
 */
class Light {
    public:
        // returns data to be interpreted by a shader storage buffer object (SSBO; see https://www.khronos.org/opengl/wiki/Shader_Storage_Buffer_Object)
        virtual vector<float> parseData();
};

/* ----- LIGHT INTERPRETATION TABLE ----- *\
Always      ID      Intensity
Index       0       1       2       3       4       5       6       7       8       9
DirLight    0       INTSTY  dir.x   dir.y   dir.z   0       0       0       0       0
PntLight    1       INTSTY  pos.x   pos.y   pos.z   0       0       0       0       0
SptLight    2       INTSTY  pos.x   pos.y   pos.z   dir.x   dir.y   dir.z   cutoff  fade
\* -------------------------------------- */

/**
 * @brief Directional class of light
 */
class DirLight : public Light {
    public:

};

/**
 * @brief Point class of light
 */
class PntLight : public Light {

};

/**
 * @brief Spotlight class of light
 */
class SptLight : public Light {

};

/**
 * @brief Defines a shader class to bind and store information on a set of vertex/fragment/(geometry) shaders. Geometry optional (and default null). (adpated from https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/shader.h)
 */
class Shader {
    public:
        unsigned int ID;
        
        Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = NULL) {
            string vertexCode;
            string fragmentCode;
            string geometryCode;
            std::ifstream vShaderFile;
            std::ifstream fShaderFile;
            std::ifstream gShaderFile;
            
            // ensure ifstream objects can throw exceptions:
            vShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
            fShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
            gShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
            
            try {
                // open files
                vShaderFile.open(vertexPath);
                fShaderFile.open(fragmentPath);
                std::stringstream vShaderStream, fShaderStream;
                
                // read file's buffer contents into streams
                vShaderStream << vShaderFile.rdbuf();
                fShaderStream << fShaderFile.rdbuf();		
                
                // close file handlers
                vShaderFile.close();
                fShaderFile.close();
                
                // convert stream into string
                vertexCode = vShaderStream.str();
                fragmentCode = fShaderStream.str();			
                
                // if geometry shader path is present, also load a geometry shader
                if(geometryPath != nullptr) {
                    gShaderFile.open(geometryPath);
                    std::stringstream gShaderStream;
                    gShaderStream << gShaderFile.rdbuf();
                    gShaderFile.close();
                    geometryCode = gShaderStream.str();
                }
            } catch (std::ifstream::failure& e) {
                std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
            }
            
            const char* vShaderCode = vertexCode.c_str();
            const char * fShaderCode = fragmentCode.c_str();
            
            // 2. compile shaders
            unsigned int vertex, fragment;
            
            // vertex shader
            vertex = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertex, 1, &vShaderCode, NULL);
            glCompileShader(vertex);
            checkCompileErrors(vertex, "VERTEX");
            
            // fragment Shader
            fragment = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragment, 1, &fShaderCode, NULL);
            glCompileShader(fragment);
            checkCompileErrors(fragment, "FRAGMENT");
            
            // if geometry shader is given, compile geometry shader
            unsigned int geometry;
            if(geometryPath != nullptr) {
                const char * gShaderCode = geometryCode.c_str();
                geometry = glCreateShader(GL_GEOMETRY_SHADER);
                glShaderSource(geometry, 1, &gShaderCode, NULL);
                glCompileShader(geometry);
                checkCompileErrors(geometry, "GEOMETRY");
            }
            
            // shader Program
            ID = glCreateProgram();
            glAttachShader(ID, vertex);
            glAttachShader(ID, fragment);
            if(geometryPath != nullptr)
                glAttachShader(ID, geometry);
            glLinkProgram(ID);
            checkCompileErrors(ID, "PROGRAM");
            
            // delete the shaders as they're linked into our program now and no longer necessery
            glDeleteShader(vertex);
            glDeleteShader(fragment);
            if(geometryPath != nullptr)
                glDeleteShader(geometry);
        }

        void use() { 
            glUseProgram(ID); 
        }
        
        void setBool(const std::string &name, bool value) const {         
            glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value); 
        }
        
        void    setInt(const std::string &name, int value) const { 
            glUniform1i(glGetUniformLocation(ID, name.c_str()), value); 
        }
        
        void setFloat(const std::string &name, float value) const { 
            glUniform1f(glGetUniformLocation(ID, name.c_str()), value); 
        }
        
        void setVec2(const std::string &name, const glm::vec2 &value) const { 
            glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
        }
        void setVec2(const std::string &name, float x, float y) const { 
            glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y); 
        }
        
        void setVec3(const std::string &name, const glm::vec3 &value) const { 
            glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
        }
        void setVec3(const std::string &name, float x, float y, float z) const { 
            glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z); 
        }
        
        void setVec4(const std::string &name, const glm::vec4 &value) const { 
            glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
        }
        void setVec4(const std::string &name, float x, float y, float z, float w)  { 
            glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w); 
        }
        
        void setMat2(const std::string &name, const glm::mat2 &mat) const {
            glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
        }
        
        void setMat3(const std::string &name, const glm::mat3 &mat) const {
            glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
        }
        
        void setMat4(const std::string &name, const glm::mat4 &mat) const {
            glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
        }

    private:
        void checkCompileErrors(GLuint shader, string type) {
            GLint success;
            GLchar infoLog[1024];
            if(type != "PROGRAM") {
                glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
                if(!success) {
                    glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                    std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << std::endl;
                }
            }
            else {
                glGetProgramiv(shader, GL_LINK_STATUS, &success);
                if(!success) {
                    glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                    std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << std::endl;
                }
            }
        }
};

/**
 * @brief Defines a mesh including sets of vertices, indices, and texture structs
 */
class Mesh {
    public:
        // mesh data
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

        Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures) {
            this->vertices = vertices;
            this->indices = indices;
            this->textures = textures;

            setupMesh();
        }

        void draw(Shader* shader) {
            unsigned int diffuseNr = 1;
            unsigned int specularNr = 1;
            for(unsigned int i = 0; i < textures.size(); i++) {
                glActiveTexture(GL_TEXTURE0 + i); // activate proper texture unit before binding
                // retrieve texture number
                string number;
                string name = textures[i].type;
                if(name == "texture_diffuse")
                    number = std::to_string(diffuseNr++);
                else if(name == "texture_specular")
                    number = std::to_string(specularNr++);

                shader->setInt(("material." + name + number).c_str(), i);
                glBindTexture(GL_TEXTURE_2D, textures[i].id);
            }
            glActiveTexture(GL_TEXTURE0);

            // draw mesh
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        } 
    private:
        // render data
        unsigned int VAO, VBO, EBO;

        void setupMesh() {
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);
        
            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);

            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);  

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

            // vertex positions
            glEnableVertexAttribArray(0);	
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
            
            // vertex normals
            glEnableVertexAttribArray(1);	
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
            
            // vertex texture coords
            glEnableVertexAttribArray(2);	
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

            glBindVertexArray(0);
        }
};

/**
 * @brief Defines a mesh including sets of vertices, indices, and texture structs
 */
class Model {
    public:
        // model data 
        vector<Texture> textures_loaded;
        vector<Mesh> meshes;
        string directory;
        bool gammaCorrection;

        // constructor, expects a filepath to a 3D model.
        Model(string const &path, bool gamma = false) : gammaCorrection(gamma) {
            loadModel(path);
        }

        // draws the model, and thus all its meshes
        void draw(Shader* shader) {
            for(unsigned int i = 0; i < meshes.size(); i++)
                meshes[i].draw(shader);
        }
    
    private:
        // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
        void loadModel(string const &path) {
            // read file via ASSIMP
            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
            
            // check for errors
            if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
                std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
                return;
            }
            
            // retrieve the directory path of the filepath
            directory = path.substr(0, path.find_last_of('/'));

            // process ASSIMP's root node recursively
            processNode(scene->mRootNode, scene);
        }

        // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
        void processNode(aiNode *node, const aiScene *scene) {
            // process each mesh located at the current node
            for(unsigned int i = 0; i < node->mNumMeshes; i++) {
                // the node object only contains indices to index the actual objects in the scene. 
                // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
                aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
                meshes.push_back(processMesh(mesh, scene));
            }
            
            // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
            for(unsigned int i = 0; i < node->mNumChildren; i++) {
                processNode(node->mChildren[i], scene);
            }
        }

        Mesh processMesh(aiMesh *mesh, const aiScene *scene) {
            // data to fill
            vector<Vertex> vertices;
            vector<unsigned int> indices;
            vector<Texture> textures;

            // walk through each of the mesh's vertices
            for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
                Vertex vertex;
                glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
                
                // positions
                vector.x = mesh->mVertices[i].x;
                vector.y = mesh->mVertices[i].y;
                vector.z = mesh->mVertices[i].z;
                vertex.position = vector;
                
                // normals
                if (mesh->HasNormals()) {
                    vector.x = mesh->mNormals[i].x;
                    vector.y = mesh->mNormals[i].y;
                    vector.z = mesh->mNormals[i].z;
                    vertex.normal = vector;
                }

                // texture coordinates
                if(mesh->mTextureCoords[0]) {
                    glm::vec2 vec;
                    // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
                    // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                    vec.x = mesh->mTextureCoords[0][i].x; 
                    vec.y = mesh->mTextureCoords[0][i].y;
                    vertex.texCoords = vec;
                    // tangent
                    vector.x = mesh->mTangents[i].x;
                    vector.y = mesh->mTangents[i].y;
                    vector.z = mesh->mTangents[i].z;
                    vertex.tangent = vector;
                    // bitangent
                    vector.x = mesh->mBitangents[i].x;
                    vector.y = mesh->mBitangents[i].y;
                    vector.z = mesh->mBitangents[i].z;
                    vertex.bitangent = vector;
                }
                else
                    vertex.texCoords = glm::vec2(0, 0);

                vertices.push_back(vertex);
            }

            // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
            for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
                aiFace face = mesh->mFaces[i];
                // retrieve all indices of the face and store them in the indices vector
                for(unsigned int j = 0; j < face.mNumIndices; j++)
                    indices.push_back(face.mIndices[j]);
            }
           
            // process materials
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];    
            // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
            // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
            // Same applies to other texture as the following list summarizes:
            // diffuse: texture_diffuseN
            // specular: texture_specularN
            // normal: texture_normalN

            // 1. diffuse maps
            vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

            // 2. specular maps
            vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

            // 3. normal maps
            std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
            textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

            // 4. height maps
            std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
            textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
            
            // return a mesh object created from the extracted mesh data
            return Mesh(vertices, indices, textures);
        }

        // checks all material textures of a given type and loads the textures if they're not loaded yet.
        // the required info is returned as a Texture struct.
        vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName) {
            vector<Texture> textures;
            for(unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
                aiString str;
                mat->GetTexture(type, i, &str);

                // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
                bool skip = false;
                for(unsigned int j = 0; j < textures_loaded.size(); j++) {
                    if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
                        textures.push_back(textures_loaded[j]);
                        skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                        break;
                    }
                }
                if(!skip) {
                    Texture texture;
                    texture.id = textureFromFile(str.C_Str(), this->directory);
                    texture.type = typeName;
                    texture.path = str.C_Str();
                    textures.push_back(texture);
                    textures_loaded.push_back(texture);
                }
            }
            return textures;
        }
};

/**
 * @brief Flips an SDL_Surface vertically such that texture is aligned properly in the final result
 * 
 * @param surface Surface of which to flip
 */
inline void flipSurface(SDL_Surface* surface) {
    SDL_LockSurface(surface);
    
    int pitch = surface->pitch; // row size
    char* temp = new char[pitch]; // intermediate buffer
    char* pixels = (char*) surface->pixels;
    
    for(int i = 0; i < surface->h / 2; ++i) {
        // get pointers to the two rows to swap
        char* row1 = pixels + i * pitch;
        char* row2 = pixels + (surface->h - i - 1) * pitch;
        
        // swap rows
        memcpy(temp, row1, pitch);
        memcpy(row1, row2, pitch);
        memcpy(row2, temp, pitch);
    }
    
    delete[] temp;

    SDL_UnlockSurface(surface);
}

/**
 * @brief Loads a texture from file
 * 
 * @param path File name/path to file from directory
 * @param directory Directory path to file/to path
 * @param gamma Load with gamma or not
 * @return unsigned int representing the loaded texture ID
 */
inline unsigned int textureFromFile(const char *path, const string &directory, bool gamma) {
    string filename = string(path);
    filename = directory + '/' + filename;

    SDL_Surface* surf = IMG_Load(filename.c_str());
    flipSurface(surf);
    if (surf == NULL) {
        SDL_Log("Unable to initialize texture: %s\n", IMG_GetError()); return 0;
    }

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height;
    void *data = surf->pixels;
    width = surf->w;
    height = surf->h;

    GLenum format;
    // TODO: Reimplement texture format detection accurately
    /*if (nrComponents == 1)
        format = GL_RED;
    else if (nrComponents == 3)
        format = GL_RGB;
    else if (nrComponents == 4)
        format = GL_RGBA;*/
    format = GL_RGB;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    SDL_FreeSurface(surf);
    
    return textureID;
}

/**
 * @brief Loads a cubemap from a list of file paths
 * 
 * @param faces List of paths to cubemap sides
 * @return unsigned int 
 */
inline unsigned int loadCubemap(vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    
    int width, height;
    for (unsigned int i = 0; i < faces.size(); i ++) {
        SDL_Surface* surf = IMG_Load(faces.at(i).c_str());
        if (surf == NULL) {
            SDL_Log("Unable to initialize texture: %s\n", IMG_GetError()); return 0;
        }
        //flipSurface(surf);

        void *data = surf->pixels;
        width = surf->w;
        height = surf->h;
        
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        SDL_FreeSurface(surf);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}


#endif