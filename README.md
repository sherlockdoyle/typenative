# TypeNative

TypeNative is yet another programming language I'm creating. I'm doing this as a project to learn how to build compilers and design languages. Right now, it's just a hobby, but I want to turn it into a serious project later.

My goal is for TypeNative to be almost the same as **TypeScript**. The key difference is that it **compiles directly to machine code**. This means it doesn't need an interpreter or a runtime (like Node or Deno). You get a single runnable file, just like you do from C or C++.

---

## Features

The plan is for TypeNative to support almost all of TypeScript's features. I'm still working on implementing everything.

### What's Not Supported

These are features from TypeScript that TypeNative will *not* have:

  * **`undefined`**: TypeNative doesn't have `undefined`. Use `null` instead. The `?` operator makes a type nullable.
  * **`prototype`**: There are no `prototype` or `__proto__` objects.
  * **`typeof`**: The `typeof` operator returns the actual type, not a string (like "string" or "number").
  * **`$` in variables**: TypeNative doesn't support `$` in variable names. It's used internally. It's also used for compile time variables.

### Extra Features

TypeNative also adds some features that TypeScript doesn't have. My main goal is to stay as close to TypeScript as possible. If TypeScript/JavaScript later adds a feature that clashes with one of these, the official TypeScript way will win. I'll find a new way to write the extra feature.

  * **Different number types**: Adds more specific number types like signed integers (`i8`, `i16`, `i32`, `i64`, `i128`), unsigned integers (`u8`, `u16`, `u32`, `u64`, `u128`), characters (`c8`, `c16`, `c32`), and floats (`f32`, `f64`, `f128`). **Complex** numbers are also supported.
  * **`valType` and `refType`**: `valType` objects are passed by value (copied), while `refType` objects are passed by reference (as a pointer). All the new number types are `valType`s.
  * **Custom number suffix**
    ```ts
    @$literal  // syntax is not yet final
    function j(x: f64): complex { return new complex(0, x); }
    let c = 1j;
    //  ^? let c: complex
    ```
  * **Function/method overloading**
    ```ts
    function add(a: number, b: number): number { return a + b; }
    function add(a: string, b: string): string { return a + b; }
    ```
  * **Operator overloading**
    ```ts
    class Point {
      [Symbol.add](that: Point): Point {}
    }
    (new Point()) + (new Point());
    ```
  * **Explicit `throws`**
    ```ts
    function f(): string, throws IOException {}
    try {
      f();
    } catch (e: IOException) {
      // do something
    }
    ```
  * **Multiple inheritance**
    ```ts
    class Derived extends Base1, Base2 {}
    ```
  * **Nested classes**
    ```ts
    class Outer {
      class Inner {}
      static class StaticInner {}
    }
    (new Outer()).new Inner();
    new Outer.StaticInner();
    ```
  * **Extension functions**
    ```ts
    function foo(this: string) { return "bar"; }  // The first parameter is `this`
    "...".foo();
    ```
  * **Generic specialization**: Lets you make special versions of a generic class for certain types (like C++ template specialization).
    ```ts
    class Array<T> {}
    class Array<T: number> extends Array<T> {}  // 'extends' is used to inherit default fields

    class Map<K, V> {}
    class Map<K: string, V> extends Map<K, V> {}  // Partial specialization
    ```
  * **Inferring parameter types**
    ```ts
    function f(a: infer) { return a.toString(); }
    //         ^? (parameter) a: { toString(): unknown; }
    ```
  * **Explicit variance**: You can manually set the variance of a type: `out` (covariant), `in` (contravariant), `in out` (bivariant), and `in-out` (invariant, syntax is not yet final).
  * **Annotations**: Annotations can be added with `@$A(...)`.
  * **Effects**: Functions can have "effects" (sometimes called "coloring"). Effects are added with `@$E(...)`.
    ```ts
    const pure = new Effect('out');  // Callee constraining
    const async = new Effect('in');  // Caller constraining
    ```
    A `$E(pure)` function can only call other `$E(pure)` functions. An `$E(async)` function can only be called by other `$E(async)` functions.
  * **Macros and compile time variables**: Compile time variables only exist at compile time. Macros are special compile time functions that can be used to generate code.
    ```ts
    const $compileTimeNumber = 1;
    function $macro() {
      return "let x = " + $compileTimeNumber + ";";
    }
    @$comptime
    function compileTimeOnlyFunction() {}
    $comptime: {
      // compile time block
    }
    $comptime: if ($compileTimeNumber == 1) {
      // compile time conditional
    }
    const $compileTimePrint = $comptime(print);
    ```
