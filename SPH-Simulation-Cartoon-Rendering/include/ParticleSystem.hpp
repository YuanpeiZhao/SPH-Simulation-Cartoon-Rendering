#ifndef _PARTICLESYSTEM_
#define _PARTICLESYSTEM_

#ifndef _GL3W_
#define _GL3W_
  #include <GL/gl3w.h>
  #include <GLFW/glfw3.h>
#endif

#define WORK_GROUP_SIZE 256
#define GLM_FORCE_RADIANS

#include <iostream>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "ShaderManager.hpp"
#include "ParticleBuffer.hpp"
#include "ParticleTexture.hpp"
#include "Attractor.hpp"
#include "GLFWWindow.hpp"
#include "GLFWInput.hpp"
#include "Timer.hpp"
#include "FirstPersonCamera.hpp"
#include "Camera.hpp"
#include "ConfigLoader.hpp"

struct Vertex{
  glm::vec4 pos;
};

class ParticleSystem{
private:
  GLFWWindow  _window;
  GLFWInput   _input;
  Camera<FirstPersonCamera> _camera;
  Attractor   _attractor;
  ParticleBuffer  _particleBuffer;
  ParticleTexture _particleTexture;
  GLuint  _vertexUVBufferID;
  GLuint  _vertexArrayID;
  GLuint  _ppVertexBufferID;
  GLuint  _ppVertexArrayID;
  GLuint  _depthMap;
  GLuint  _depthMapFBO;
  float   _quadLength;
  bool    _showFPS;
  GLuint  _computeProgID, _shaderProgID, _ppShaderProgID;
  float   _deltaTime;
  float   _tmpDeltaTime;

  ShaderManager  _shaderManager;
  
  
  struct ComputeShaderUniformLocations {
    GLuint frameTimeDiff;
    GLuint pass;
    ComputeShaderUniformLocations() : frameTimeDiff(0), pass(0) {}
  } _csLocations;
  
  struct ParticleShaderUniformLocations {
    GLuint viewMatrix;
    GLuint camPos;
    GLuint time;
    ParticleShaderUniformLocations() : viewMatrix(0), camPos(0), time(0) {}
  } _psLocations;

  void render(double, double);
  void initDepthMap();
  void drawDepthMap(double);
  void postProcessing();
  void deleteParticleSystem() noexcept;
  
public:
  ParticleSystem() = delete;
  ParticleSystem(const Config&);
  ParticleSystem(ParticleSystem&) = delete;
  ParticleSystem(ParticleSystem&&) = delete;
  ParticleSystem& operator=(ParticleSystem&) = delete;
  ParticleSystem& operator=(ParticleSystem&&) = delete;
  ~ParticleSystem();

  void initialize();
  void run();
  void resize(int width, int height);
};


#endif
