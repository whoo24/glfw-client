
#include "stdafx.h"
#include <plog/Log.h>
#include <plog/Appenders/ConsoleAppender.h>
#include "shared/glew-2.1.0/include/GL/glew.h"
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <fstream>
#include <string>
#include <cstdlib>
#include "fmt/format.h"
#include "linmath.h"

#define BUFFER_OFFSET(x) (reinterpret_cast<const void*>(x))

void error_callback(int error, const char* description) {
  LOG(plog::error) << "Error: " << description;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }
}

void UpdateFpsCounter(GLFWwindow* window) {
  static double previous_seconds = glfwGetTime();
  static int frame_count;
  double current_seconds = glfwGetTime();
  double elapsed_seconds = current_seconds - previous_seconds;
  if (elapsed_seconds > 0.25) {
    previous_seconds = current_seconds;
    double fps = (double)frame_count / elapsed_seconds;
    glfwSetWindowTitle(window, fmt::format("opengl @ fps: {0}", fps).c_str());
    frame_count = 0;
  }
  frame_count++;
}

static const struct {
  float x, y, z;
} vertices[3] = {
  { -0.6f, -0.4f, 0.0f},
  {  0.6f, -0.4f, 0.0f},
  {   0.f,  0.6f, 0.0f}
};

float points[] = {
  0.0f,  0.5f,  0.0f,
  0.5f, -0.5f,  0.0f,
  -0.5f, -0.5f,  0.0f
};

void SetEnvironment(int argc, const char * const argv[]) {
  assert(argc >= 1);

  size_t len = 0;
  char *working_dir = nullptr;
  _dupenv_s(&working_dir, &len, "WorkingDir");
  if (working_dir == nullptr) {
    std::string dir(argv[0]);
    std::replace(dir.begin(), dir.end(), '/', '\\');
    auto pos = dir.rfind('\\');
    assert(pos != std::string::npos);
    dir = std::string(argv[0], pos + 1);
    auto cur_path = fmt::format("WorkingDir={0}", dir.c_str());
    _putenv(cur_path.c_str());
  }
}

std::string GetEnvironment(const std::string& variable) {
  size_t len = 0;
  char *value = nullptr;
  _dupenv_s(&value, &len, variable.c_str());
  return value;
}

void LoadFile(const std::string& filepath, std::string* out) {
  std::ifstream stream(filepath);
  out->insert(out->begin(), std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

bool CheckShader(GLuint shader) {
  GLint state;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &state);
  if (state != GL_FALSE) {
    return true;
  }
  int infolog_length = 0;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infolog_length);
  if (infolog_length > 0) {
    int charsWritten = 0;
    std::string info_log;
    info_log.resize(infolog_length);
    glGetShaderInfoLog(shader, infolog_length, &charsWritten, &info_log[0]);
    LOG(plog::error) << info_log;
  }
  return false;
}

void main(int argc, const char * const argv[]) {
  plog::RollingFileAppender<plog::TxtFormatter> file_logger("log.txt", 8000, 3); // Create the 1st appender.
  plog::ConsoleAppender<plog::TxtFormatter> console_logger; // Create the 2nd appender.
  plog::init(plog::debug, &console_logger).addAppender(&file_logger);
  SetEnvironment(argc, argv);
  glfwSetErrorCallback(error_callback);
  if (!glfwInit()) {
    exit(EXIT_FAILURE);
  }
  GLFWwindow* window = glfwCreateWindow(640, 480, "Simple example", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
  glfwMakeContextCurrent(window);
  glfwSetKeyCallback(window, key_callback);
  glfwWindowHint(GLFW_SAMPLES, 4);
  // start GLEW extension handler
  glewExperimental = GL_TRUE;
  glewInit();
  // get version info
  const std::string renderer(reinterpret_cast<const char*>(glGetString(GL_RENDERER))); // get renderer string
  const std::string version(reinterpret_cast<const char*>(glGetString(GL_VERSION))); // version as a string
  LOG_INFO << "Renderer: " << renderer;
  LOG_INFO << "OpenGL version supported " << version;
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glfwSwapInterval(1);
  std::string vs;
  LoadFile(fmt::format("{0}shader_vs.glsl", GetEnvironment("WorkingDir")), &vs);
  const char* vertex_shader_text = vs.c_str();
  std::string ps;
  LoadFile(fmt::format("{0}shader_ps.glsl", GetEnvironment("WorkingDir")), &ps);
  const char* fragment_shader_text = ps.c_str();
  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
  glCompileShader(vertex_shader);
  if (!CheckShader(vertex_shader)) {
    exit(EXIT_FAILURE);
  }
  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
  glCompileShader(fragment_shader);
  if (!CheckShader(fragment_shader)) {
    exit(EXIT_FAILURE);
  }
  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);
  GLuint vertex_buffer;
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(float), points, GL_STATIC_DRAW);
  GLuint vao = 0;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
  GLuint vertex_buffer_2nd;
  glGenBuffers(1, &vertex_buffer_2nd);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_2nd);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  GLuint vao2 = 0;
  glGenVertexArrays(1, &vao2);
  glBindVertexArray(vao2);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_2nd);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
  glDisable(GL_CULL_FACE);
  glFrontFace(GL_CW);
  glPolygonMode(GL_FRONT, GL_FILL);
  glPolygonMode(GL_BACK, GL_LINE);
  while(!glfwWindowShouldClose(window)) {
    UpdateFpsCounter(window);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(program);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(vao2);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glfwPollEvents();
    glfwSwapBuffers(window);
  }
  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}
