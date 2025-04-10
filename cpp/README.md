# JsonLogic for C++

This is an implementation for [JsonLogic](https://jsonlogic.com/) for C++. The API uses the Boost
JSON implementation (e.g.,
[Boost 1.82](https://www.boost.org/doc/libs/1_82_0/libs/json/doc/html/index.html)).

The library is designed to follow the type conversion rules of the reference JsonLogic
implementation.

## Compile and Install

The library can be installed using cmake.

    mkdir build
    cd build
    cmake ..
    make

## Use

The simplest way is to create Json rule and data options and call jsonlogic::apply.

    #include <jsonlogic/logic.hpp>

    boost::json::value rule = ..;
    boost::json::value data = ..;
    jsonlogic::any_expr res = jsonlogic::apply(rule, data);
    std::cout << res << std::endl;

See examples/testeval.cc for the complete sample code.

To evaluate a rule multiple times, it may be beneficial to convert the Json object into JsonLogic's
internal expression representation.

    #include <jsonlogic/logic.hpp>

    boost::json::value rule = ..;
    std::vector<boost::json::value> massdata = ..;
    jsonlogic::create_logic logic = jsonlogic::create_logic(rule, data);

    for (boost::json::value data : massdata)
    {
        jsonlogic::variable_accessor varlookup = jsonlogic::data_accessor(std::move(data));

        std::cout << jsonlogic.apply(logic.syntax_tree(), std::move(varlookup)) << std::endl;
    }

## Python Companion

[Clippy](https://github.com/LLNL/clippy) is a companion library for Python that creates Json objects
that can be evaluated by JsonLogic.

## Authors

Peter Pirkelbauer (pirkelbauer2 at llnl dot gov)

## License

CLIPPy is distributed under the MIT license.

See LICENSE-MIT, NOTICE, and COPYRIGHT for details.

SPDX-License-Identifier: MIT

## Release

LLNL-CODE-818157
