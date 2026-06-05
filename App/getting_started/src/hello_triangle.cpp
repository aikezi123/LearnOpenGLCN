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



    // —————————— 2. 创建并编译顶点着色器 ————————————
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


    // —————————— 3. 片段着色器 ————————————
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


    // —————————— 4. 着色器程序 Shader Program ————————————
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
    // 作用：删除单独的 shader object。
    // 为什么链接后可以删除：
    //   shader 已经被链接进 shaderProgramID 了。
    //   后续绘制只需要 shaderProgramID。
    //   单独的 vertexShaderID 和 fragmentShaderID 不再需要保留。
    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);

    // glUseProgram(program)
    // 作用：把 shaderProgramID 设置为当前 OpenGL Context 使用的着色器程序。后续 draw call 会使用这个 program 进行渲染。
    // 注意：
    //   这句不是“立刻运行程序”。
    //   它只是选择当前使用哪个 GPU 程序。
    //   真正运行发生在后面的 glDrawArrays 或 glDrawElements。
    glUseProgram(shaderProgramID);


// —————————— 5. 创建 VAO 和 VBO ——————————
//
// VBO：Vertex Buffer Object，顶点缓冲对象。
//      负责存储真正的顶点数据。
//      也就是把 CPU 中的 vertices 数组上传到 GPU 侧的缓冲中。
//
// VAO：Vertex Array Object，顶点数组对象。
//      负责记录“VBO 中的数据应该怎么被解释”。
//      它本身不主要存顶点数据，而是记录顶点属性配置规则。
//
// 简单理解：
//   VBO = 存数据
//   VAO = 存规则
//
// GPU 绘制主要从显存里读取数据，而不是从 CPU 中读取数据。
// CPU 内存里的数据生命周期可能很短，比如函数结束后局部数组就失效。
// GPU 渲染通常是异步的。调用 OpenGL 绘图命令时，GPU 可能稍后才真正执行。
// 把数据一次性上传到 GPU，后面重复绘制时就不需要每一帧都从 CPU 传一次，效率更高。

unsigned int VAO;
unsigned int VBO;

// 创建 1 个 VAO。
// VAO 用来记录后面的顶点属性配置。
glGenVertexArrays(1, &VAO);

// 创建 1 个 VBO。
// VBO 用来保存真正的顶点数据。
glGenBuffers(1, &VBO);


// —————————— 6. 绑定 VAO，开始记录顶点属性配置 ——————————
//
// 注意：
//   这里要先绑定 VAO。
//   因为后面的 glVertexAttribPointer 和 glEnableVertexAttribArray
//   都会被当前绑定的 VAO 记录下来。
//
// 当前绑定 VAO 后，可以理解为：
//   “接下来关于顶点属性的配置，都记录到这个 VAO 里面。”
glBindVertexArray(VAO);


// —————————— 7. 绑定 VBO，并把 CPU 顶点数据上传到 GPU ——————————

// 绑定顶点缓冲对象 VBO 到 OpenGL Context 当前的 GL_ARRAY_BUFFER 绑定点上。
//
// GL_ARRAY_BUFFER 表示：
//   当前用于顶点属性数据的缓冲对象绑定点。
//
// 绑定后，后续对 GL_ARRAY_BUFFER 的操作，
// 都会作用到当前绑定的 VBO 上。
glBindBuffer(GL_ARRAY_BUFFER, VBO);

// 把 CPU 的顶点数据上传到当前绑定在 GL_ARRAY_BUFFER 上的 VBO 中。
// 参数 1：GL_ARRAY_BUFFER,操作当前绑定在 GL_ARRAY_BUFFER 上的缓冲对象，也就是 VBO。
// 参数 2：sizeof(vertices),要上传的数据大小，单位是字节。
// 参数 3：vertices,CPU 内存中顶点数组的首地址。
// 参数 4：GL_STATIC_DRAW,使用提示，表示数据基本不变，并且主要用于绘制。
// 作用：
//   在 GPU 侧给当前绑定的 VBO 分配存储空间，
//   并把 CPU 中的 vertices 数据复制进去。
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);


// ———————————— 8. 链接顶点属性 ————————————
//
// 前面我们已经把 vertices 数组上传到了 VBO 中。
// 但是 VBO 里本质上只是一段连续的 float 数据：
//
//   -0.5, -0.5, 0.0,   0.5, -0.5, 0.0,   0.0, 0.5, 0.0
//
// OpenGL 默认不知道这些 float 应该如何解释：
//   1. 每几个 float 组成一个顶点？
//   2. 这些 float 表示位置、颜色，还是纹理坐标？
//   3. 相邻两个顶点之间间隔多少字节？
//   4. shader 中哪个输入变量接收这些数据？
//
// 所以需要通过 glVertexAttribPointer 告诉 OpenGL：
//   当前绑定的 VBO 中的数据，应该如何送入顶点着色器的某个输入属性。
//
// 顶点着色器中有：
//
//   layout (location = 0) in vec3 aPos;
//
// 这表示：
//   顶点着色器需要一个输入变量 aPos；
//   aPos 是 vec3，也就是 3 个 float；
//   它对应的顶点属性位置是 location = 0。
//
// 下面这句 glVertexAttribPointer 就是在配置 location = 0 这个顶点属性。
//
// glVertexAttribPointer(index, size, type, normalized, stride, pointer)
//
// 参数 1：0。要配置的顶点属性位置。对应 shader 里的 layout(location = 0)。
// 参数 2：3。每个顶点属性由 3 个分量组成。这里表示每个顶点位置由 x、y、z 三个 float 组成。对应 shader 里的 vec3 aPos。
// 参数 3：GL_FLOAT。每个分量的数据类型是 float。因为 vertices 数组的类型是 float[]。
// 参数 4：GL_FALSE。是否把数据归一化。对 float 类型的顶点坐标来说，一般不需要归一化，所以写 GL_FALSE。
// 参数 5：3 * sizeof(float)。stride，步长。表示从当前顶点的位置数据开始，到下一个顶点的位置数据开始，间隔多少字节。当前每个顶点只有 3 个 float，所以步长是 3 * sizeof(float)。
// 参数 6：(void*)0。offset，偏移量。表示 location = 0 这个属性从 VBO 的第几个字节开始读取。当前 vertices 一开始就是位置数据，所以偏移量是 0。
// 注意：
//   glVertexAttribPointer 会读取“当前绑定在 GL_ARRAY_BUFFER 上的 VBO”，
//   并把这个 VBO 和 location = 0 的解析规则记录到当前绑定的 VAO 中。
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

// 启用 location = 0 的顶点属性。
//
// glVertexAttribPointer 只是告诉 OpenGL location = 0 应该如何读取数据，
// 但默认情况下顶点属性数组是关闭的。
//
// 必须调用 glEnableVertexAttribArray(0) 启用它，
// 顶点着色器中的 aPos 才能真正接收到 VBO 中的数据。
//
// 这个“启用状态”也会被当前绑定的 VAO 记录下来。
glEnableVertexAttribArray(0);


// —————————— 9. 可选解绑 ——————————
//
// 解绑 GL_ARRAY_BUFFER。
// 这是允许的，因为 glVertexAttribPointer 已经把：
//   当前 VBO + location = 0 的解析规则
// 记录到了当前 VAO 中。
glBindBuffer(GL_ARRAY_BUFFER, 0);

// 解绑 VAO。
// 这样后续其他 VAO/VBO 操作不会意外修改当前 VAO。
//
// 后面绘制时，只需要重新：
//   glBindVertexArray(VAO);
// 就可以恢复这里记录的顶点属性配置。
glBindVertexArray(0);

    return 0;
}

