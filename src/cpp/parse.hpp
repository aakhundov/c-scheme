#ifndef PARSE_H_
#define PARSE_H_

#include <iostream>
#include <memory>

#include "value.hpp"

std::shared_ptr<value> parse_values(std::istream& is);

#endif  // PARSE_H_
