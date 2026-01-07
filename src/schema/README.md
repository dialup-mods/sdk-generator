## Schema

The `Schema` defines the minimal set of Unreal Engine classes and functions to enable SDK generation.

`Schema.h` and `Schema.cpp` will be included with the final generated output. This means you can add custom code to your SDK **but it is generally a terrible idea**. Prefer to keep your SDK as close to the engine as possible, as this is the interface between your plugin and the engine. Clean separation of concerns. To use custom code across your plugins, compile in a separate module. If you must slap unrelated code in Schema, at least namespace it.

### SchemaParser

Some of these classes may be minimal, enough to generate the SDK; however, the generated output may contain more complete definitions.

The SchemaParser module allows us to declare the prerequisite definitions and combine them with the generated output, without having to store a separate copy *(no more `PiecesOfCode`!)*

In `Schema.h`, you may modify the resulting generated SDK by using the following doxygen-style comments:

```c++
/// @replace
//    replace the definition in the generated output
/// @final
//    leave this definition in Schema.h, do not output the generated class
/// @inject-methods
//    inject methods into the generated output
//    use @inject-methods to signal the start of capture
//    note: this will delete the class from Schema.h as you would expect
/// @inject-end
//    signal the end of method capture for injection
```

An example of usage can be seen in the default game profile, at `/config/default/Schema.h`.
