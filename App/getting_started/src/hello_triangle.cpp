#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>


void createAndCompileVertexShader() {
    
}

// ——————————————————————————————————————————————————————
// ———————————————— OpenGL Context 理解 —————————————————
// OpenGL Context负责记录：当前用哪些对象和状态来渲染
//   Texture / VBO / VAO / Shader
//   是渲染过程中用到的资源和规则
//   Framebuffer
//   保存最终渲染出来的图像结果
//   OpenGL 可以理解为一套状态机式的图形 API。
//   程序当前使用的 OpenGL 状态由 OpenGL Context 管理。
//
// OpenGL Context 中有很多“绑定点/插槽”，例如：
//   GL_ARRAY_BUFFER         ：当前顶点属性缓冲绑定点
//   GL_ELEMENT_ARRAY_BUFFER ：当前索引缓冲绑定点
//   当前 VAO                 ：当前顶点数组对象
//   当前 Shader Program      ：当前使用的着色器程序
//   当前纹理绑定点            ：当前使用的纹理对象
//
// glGenBuffers 创建的是 OpenGL 缓冲对象 ID。
// glBindBuffer 并不会上传数据，它只是把某个缓冲对象设置为某个绑定点的“当前对象”。
// glBufferData 才会给当前绑定的缓冲对象分配存储，并把 CPU 数据复制进去。
//
// 可以近似理解为：
// OpenGL Context
// ├─ GL_ARRAY_BUFFER 当前绑定点       -> 当前绑定的 VBO
// ├─ GL_ELEMENT_ARRAY_BUFFER 当前绑定点 -> 当前绑定的 EBO
// ├─ 当前 VAO                         -> 当前使用的 VAO
// ├─ 当前 Shader Program              -> 当前使用的 Shader Program
// ├─ 当前纹理绑定点                    -> 当前使用的 Texture
// └─ 已创建的 OpenGL 对象
//    ├─ VBO 1
//    ├─ VBO 2
//    ├─ VAO 1
//    ├─ Shader Program 1
//    └─ Texture 1
//
// VBO 对象的数据存储由 OpenGL 驱动管理，通常位于 GPU 显存中。
// ——————————————————————————————————————————————————————

int hello_triangle()
{
    // —————————— 1. CPU内存中那个创建顶点坐标 ————————————
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.0f,  0.5f, 0.0f
    };

    // —————————— 2. 将CPU中的顶点数据上传到GPU ————————
    // ——————————— GPU绘制主要从显存里读取数据，而不是从CPU中读取数据，并且CPU内存里的数据生命周期可能很短，函数结束数组就失效了——————————
    // ——————————— GPU渲染是异步的。调用OpenGL绘图时，GPU可能稍后执行。如果GPU依赖CPU上的临时数据很不稳定。 ——————————
    // ——————————— 把数据一次性上传到GPU，后面重复绘制时就不需要每一帧都从CPU传一次，效率更高 —————————


    // 生成带有缓冲ID的顶点缓冲对象(VBO)
    unsigned int VBO;       // 生成编号
    glGenBuffers(1, &VBO);  // 创建一个缓冲对象，并把它的ID写入VBO。每一个顶点缓冲对象都有一个对应的ID。

    // 绑定顶点缓冲对象(VBO)绑定到OpenGL Context当前GL_ARRAY_BUFFER插槽上。
    glBindBuffer(GL_ARRAY_BUFFER, VBO);    // GL_ARRAY_BUFFER表示这个缓冲区将用来存储顶点属性数据
    
    // 把CPU的顶点数据上传到OpenGL Context当前GL_ARRAY_BUFFER绑定的顶点缓冲对象(VBO)上去
    // 参数 1：GL_ARRAY_BUFFER，操作当前绑定在 GL_ARRAY_BUFFER 上的缓冲对象，也就是 VBO。
    // 参数 2：sizeof(vertices)，要上传的数据大小，单位是字节。
    // 参数 3：vertices，CPU 内存中顶点数组的首地址。
    // 参数 4：GL_STATIC_DRAW，使用提示，表示数据基本不变，并且主要用于绘制。
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);  // 在GPU中给当前绑定的VBO分配内存，并把CPU中的vertices数据复制进去


    // —————————— 3. 创建并编译顶点着色器 ————————————
    // 这个过程可以理解为我写了一个 .cpp 文件，编译器把它编译成 .obj，但是它还没有被链接进最终程序，也没有被 main() 调用，所以它目前只是“编译好了”，还没有真正参与运行
    // 顶点着色器是运行在 GPU 渲染管线中的一段小程序。
    // 它会对每一个输入顶点执行一次。
    // 这里的顶点着色器只做一件事：
    // 把输入的顶点坐标 aPos 直接写入 OpenGL 内置变量 gl_Position。
    const char *vertexShaderSource = R"(
        #version 330 core

        layout (location = 0) in vec3 aPos;

        void main()
        {
            gl_Position = vec4(aPos, 1.0);
        }
    )";
        

    // 创建一个顶点着色器对象。
    // GL_VERTEX_SHADER 表示这个 shader 对象的类型是“顶点着色器”。
    // 返回值 vertexShaderID 是 OpenGL 分配的着色器对象 ID。
    unsigned int vertexShaderID;
    vertexShaderID = glCreateShader(GL_VERTEX_SHADER);

    // 把 GLSL 源码字符串设置到 vertexShaderID 这个着色器对象中。
    // 参数 1：vertexShader，要设置源码的着色器对象。
    // 参数 2：1，源码字符串数量。
    // 参数 3：&vertexShaderSource，源码字符串指针的地址。
    // 参数 4：NULL，表示字符串以 '\0' 结尾，不额外提供长度。
    glShaderSource(vertexShaderID, 1, &vertexShaderSource, NULL);

    // 编译 vertexShaderID 中保存的 GLSL 源码。
    // 注意：这里是“编译”，不是“运行”。
    // shader 真正运行要等到：
    // 1. 顶点着色器和片段着色器被链接成 shader program；
    // 2. 调用 glUseProgram(shaderProgram) 使用该程序；
    // 3. 调用 glDrawArrays 或 glDrawElements 发起绘制。
    glCompileShader(vertexShaderID);

    // 检查编译是否成功。
    int success;
    char infoLog[512];

    // 查询 vertexShaderID 的编译状态。
    // 参数 1：vertexShader，要查询的着色器对象。
    // 参数 2：GL_COMPILE_STATUS，表示查询编译状态。
    // 参数 3：&success，查询结果写入 success。
    glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        // 如果编译失败，获取编译错误日志。
        // 参数 1：vertexShader，要获取日志的着色器对象。
        // 参数 2：512，日志缓冲区大小。
        // 参数 3：NULL，不需要实际日志长度。
        // 参数 4：infoLog，用来接收错误信息。
        glGetShaderInfoLog(vertexShaderID, 512, NULL, infoLog);

        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }


    // —————————— 4. 片段着色器 ————————————
    //
    // 片段着色器 Fragment Shader：
    //   运行在光栅化之后。
    //   顶点着色器确定三角形三个顶点的位置后，OpenGL 会把三角形光栅化成很多片段。
    //   片段可以先简单理解成“候选像素”。
    //   片段着色器会对每一个片段执行一次，用来计算这个片段最终的颜色。
    //
    // 这里的片段着色器逻辑很简单：
    //   不接收额外输入。
    //   每个片段都输出同一个橙色。
    const char* fragmentShaderSource = R"(
        #version 330 core

        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
        }
    )";

    // 创建一个片段着色器对象。
    //
    // glCreateShader(type)
    //
    // 参数：
    //   GL_FRAGMENT_SHADER
    //     表示创建的是“片段着色器”对象。
    //
    // 返回值：
    //   fragmentShaderID 是 OpenGL 分配的着色器对象 ID。
    //   它只是一个编号，用来代表这个 shader 对象。
    unsigned int fragmentShaderID;
    fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // 把 GLSL 片段着色器源码设置到 fragmentShaderID 这个着色器对象中。
    // glShaderSource(shader, count, string, length)
    // 参数 1：fragmentShaderID,要设置源码的着色器对象。
    // 参数 2：源码字符串数量。这里只有一段源码，所以是 1。
    // 参数 3：&fragmentShaderSource,源码字符串指针的地址。注意 glShaderSource 需要的是 const char**，所以这里传地址。
    // 参数 4：NULL,每段源码字符串的长度。传 NULL 表示源码字符串以 '\0' 结尾，由 OpenGL 自己计算长度。
    glShaderSource(fragmentShaderID, 1, &fragmentShaderSource, NULL);

    // 编译片段着色器。
    // glCompileShader(shader)
    // 参数：fragmentShaderID,要编译的片段着色器对象。
    // 作用：OpenGL 驱动会把 fragmentShaderID 中保存的 GLSL 源码,编译成 GPU 后续可以执行的着色器代码。
    // 注意：
    //   这里仍然只是“编译”，不是“运行”。
    //   片段着色器真正运行要等到：
    //   1. 它和顶点着色器一起链接成 shader program；
    //   2. 调用 glUseProgram(shaderProgram) 使用这个程序；
    //   3. 调用 glDrawArrays 或 glDrawElements 发起绘制；
    //   4. 三角形被光栅化成片段后，GPU 才会对每个片段执行这个片段着色器。
    glCompileShader(fragmentShaderID);


    // —————————— 5. 着色器程序 Shader Program ————————————
    //
    // 前面我们分别编译了：
    //   1. 顶点着色器 vertexShaderID
    //   2. 片段着色器 fragmentShaderID
    //
    // 但单独的 shader object 还不能直接用于绘制。
    // 它们需要被链接成一个完整的 shader program。
    //
    // 可以类比 C++：
    //   .cpp 文件编译后得到 .obj
    //   多个 .obj 经过链接后得到 .exe
    //
    // 对 OpenGL 来说：
    //   vertexShaderID   类似一个编译后的目标文件
    //   fragmentShaderID 类似一个编译后的目标文件
    //   shaderProgramID  类似最终链接好的 GPU 程序
    unsigned int shaderProgramID;

    // glCreateProgram()
    // 作用：创建一个着色器程序对象。
    // 返回值：
    //   返回 OpenGL 分配的 program 对象 ID。
    //   后续 attach、link、use 都通过这个 ID 操作该程序对象。
    shaderProgramID = glCreateProgram();

    // glAttachShader(program, shader)
    // 作用：把已经编译好的 shader object 附加到 shader program 上。
    // 参数 1：shaderProgramID，要附加到哪个 program。
    // 参数 2：vertexShaderID，要附加的顶点着色器对象。
    glAttachShader(shaderProgramID, vertexShaderID);

    // 把片段着色器对象也附加到同一个 shader program 上。
    glAttachShader(shaderProgramID, fragmentShaderID);

    // glLinkProgram(program)
    // 作用：把附加到 program 上的多个 shader object 链接成一个完整的 GPU 程序。
    // 对本例来说，就是把：顶点着色器 + 片段着色器，链接成一个可以用于绘制的 shader program。
    glLinkProgram(shaderProgramID);

    // glGetProgramiv(program, pname, params)
    // 作用：查询 shader program 的某个状态。
    // 参数 1：shaderProgramID，要查询的 program 对象。
    // 参数 2：GL_LINK_STATUS，查询链接状态。
    // 参数 3：&success，查询结果写入 success。
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &success);

    if (!success)
    {
        // glGetProgramInfoLog(program, maxLength, length, infoLog)
        // 作用：获取 shader program 链接失败时的错误日志。
        // 参数 1：shaderProgramID，要获取日志的 program 对象。
        // 参数 2：512，infoLog 缓冲区最大长度。
        // 参数 3：NULL，实际写入长度。这里不需要，所以传 NULL。
        // 参数 4：infoLog，接收错误日志的字符数组。
        glGetProgramInfoLog(shaderProgramID, 512, NULL, infoLog);
    }

    // glDeleteShader(shader)
    //
    // 作用：
    //   删除单独的 shader object。
    //
    // 为什么链接后可以删除：
    //   shader 已经被链接进 shaderProgramID 了。
    //   后续绘制只需要 shaderProgramID。
    //   单独的 vertexShaderID 和 fragmentShaderID 不再需要保留。
    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);

    // glUseProgram(program)
    //
    // 作用：
    //   把 shaderProgramID 设置为当前 OpenGL Context 使用的着色器程序。
    //   后续 draw call 会使用这个 program 进行渲染。
    //
    // 注意：
    //   这句不是“立刻运行程序”。
    //   它只是选择当前使用哪个 GPU 程序。
    //   真正运行发生在后面的 glDrawArrays 或 glDrawElements。
    glUseProgram(shaderProgramID);


    return 0;
}

