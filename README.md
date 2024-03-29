**Work In Progress**

# Contents

- [Contents](#contents)
- [Synopsis](#synopsis)
- [Features](#features)
- [Dependencies](#dependencies)
- [Building](#building)
  - [How To build container and execute tests](#how-to-build-container-and-execute-tests)
  - [How To include into cmake](#how-to-include-into-cmake)
  - [How To make doxygen](#how-to-make-doxygen)
  - [How To generate the code coverage report](#how-to-generate-the-code-coverage-report)
- [Usage](#usage)

# Synopsis

The yet another ORM library built on top of [SOCI](https://github.com/SOCI/soci) whithin the idea to give a simple
interface by implementing a DSL for C++ for working with SQL databases.

# Features
* Mapping C++ data structure onto DB structures
* Creation and deletion of tables by using the predefined structs in C++
* Querying and insertion (by one entity) data from tables and map the results onto C++ data struct

# Dependencies
* SOCI lib. as a submodule
* Boost Libraries >= 1.70.0 as sys. dependency
    * Boost Preprocessor
    * Boost Proto
* `gcov`, `lcov`, `genhtml`, `doxygen` are optional and serve the documentation/coverage purposes

Here is the high-level design:

<!--
```
@startuml hld

!includeurl https://raw.githubusercontent.com/RicardoNiepel/C4-PlantUML/master/C4_Container.puml

Container(customer, Customer)

System_Boundary(c1, "DSL") {
    Container(wrp, "soci-wrapper", "C++")
    Container_Ext(dsl, "Boost Proto", "C++")
}
Container_Ext(soci, "SOCI C++", "C++")
ContainerDb(db, "Database")
Rel_Down(customer, wrp, "Uses")
Rel_Right(wrp, dsl, "Uses")
Rel_Down(wrp, soci, "Uses")
Rel_Right(soci, db, "Reads/Writes")

@enduml
```
-->

![](assets/hld.svg)

# Building

`soci-wrapper` is a header-only. To use it just add the necessary `#include` line to your source code, such as the
following: 
```cpp
#include "soci-wrapper.hpp"
```

## How To build container and execute tests

There is a script added to serve the purpose. The `run.sh` script is located in the root folder of the project. The following options are availavle:
- `clear` - cleans the build directory
- `compile_container` - compiles the docker container w/ the tag `soci_wrapper`
- `compile_debug` - compiles the library in Debug mode with the tests

For the full chain execution from the scratch just invoke the following command:
```sh
./run.sh --clear --compile_container --compile_debug 

Usage: run.sh [options]

Options:
    --clear                          Clear build dir
    --compile_container              Compile the docker container
    --compile_debug                  Compile in debug mode w/ tests
```

## How To include into cmake

Adding the library as a submodule

```sh
git submodule add https://github.com/osydunenko/soci-wrapper.git 3rdParty/soci-wrapper
```

Then add a connetion to the library as a `subdirectory` into your `CMakeLists.txt`

```cmake
add_subdirectory(3rdParty/soci-wrapper)
```

And finally link to the target-executable

```cmake
target_link_libraries(
    ...
    PRIVATE
        ...
        soci_wrapper::soci_wrapper
        ...
)
```

## How To make doxygen

For the `api` docu generation you need to execute a target by using your generator passed to cmake.

The target is "excluded from all" (requires a standalone execution) and is called as `doc`

The following command shows the docu generation as a reference:

```sh
make doc
```

As a result, you may find the dir `doc` created in the `${CMAKE_CURRENT_SOURCE_DIR}` and listed the docu.

## How To generate the code coverage report

For this purposes you need to have installed some binaries of `lvoc`, `gcov` and `genhtml`

The following dependencies are taken from the `CMakeLists.txt` as a reference

```cmake
find_program(GCOV_PATH gcov)
find_program(LCOV_PATH lcov)
find_program(GENHTML_PATH genhtml)
```

So, to have the coverage results use the following set of commands

```sh
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DSW_BUILD_TESTS=ON -DSW_CPPTEST_COVERAGE=ON ../
make -j 4
make test
make coverage
```

As a result, you may find the dir `coverage` created in the `${CMAKE_CURRENT_SOURCE_DIR}` and listed the coverage
report.

# Usage

More examples are available as UTs and located [here...](https://github.com/osydunenko/soci-wrapper/tree/main/tests)

C++ structure definition and declare the struct as a persisted type

```cpp
#include "soci-wrapper.hpp"

namespace sw = soci_wrapper;

struct person
{
    int id;
    std::string name;
    std::string surname;

    bool operator==(const person &rhs) const
    {
        return id == rhs.id &&
            name == rhs.name &&
            surname == rhs.surname;
    }
};

DECLARE_PERSISTENT_OBJECT(person,
    id,
    name,
    surname
);
```

Make a connection to the Database. 

You may also use a session pool insted of a dedicated connection. Please see [here...](https://github.com/osydunenko/soci-wrapper/blob/main/tests/pool.cpp) as a reference.

```cpp
session = sw::session::connect("tst_object.db");
```

Create a table as it was specified and declared as a persisted object

```cpp
sw::ddl<person>::create_table(*session);
```

Insertion-Querying into DB and map the data then to C++ structure

```cpp
person prsn {
    .id = 20,
    .name = "name 20",
    .surname = "surname 20"
};

sw::dml::persist(*session, prsn); // storing the data

prsn = sw::dql::query_from<person>()
    .where(sw::fields_query<person>::id == 20)
    .object(*session); // querying the data by using id
```
