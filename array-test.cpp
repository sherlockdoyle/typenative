#include "src/core/ds/array.hpp"
#include "src/core/rt/AutoRef.hpp"
#include "src/core/rt/Object.hpp"
#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <iostream>
#include <ostream>
#include <string>

class Node : virtual public Object {
public:
  std::string name;
  AutoRef<Node> next, child;

  Node(const std::string &n) : name(n) { std::cout << "Constructing Node " << name << std::endl; }
  ~Node() { std::cout << "Destroying Node " << name << std::endl; }

  std::string toString() const override { return name; }

  REGISTER_CHILDREN(next, child)
};
using $Node = AutoRef<Node>;

int main() {
  auto arr1 = $Array<$Node>::make(4);
  arr1->push($Array<$Node>::make(std::initializer_list<$Node>{$Node::make("b"), $Node::make("c")}));
  arr1->unshift($Array<$Node>::make(std::initializer_list<$Node>{$Node::make("d"), $Node::make("e")}));
  std::cout << "arr1 = " << arr1->toString() << std::endl;

  auto arr2 = $Array<$Node>::make(std::initializer_list<$Node>{$Node::make("f"), $Node::make("g"), $Node::make("h")});
  std::cout << "arr2 = " << arr2->toString() << std::endl;

  auto arrConcat = arr1->concat(arr2);
  std::cout << "arrConcat = " << arrConcat->toString() << std::endl;

  auto arrSlice = arrConcat->slice(0, -1);
  std::cout << "arrSlice = " << arrSlice->toString() << std::endl;

  auto arrSplice = arrConcat->splice(2, 4, arrSlice);
  std::cout << "arrSplice = " << arrSplice->toString() << std::endl;
  std::cout << "arrConcat = " << arrConcat->toString() << std::endl;

  std::cout << (arrSplice->includes((*arrSplice)[0]) ? "true" : "false") << std::endl;
  std::cout << (arrSplice->includes($Node::make((*arrSplice)[0]->name)) ? "true" : "false") << std::endl;
  std::cout << arrSplice->indexOf((*arrSplice)[0]) << std::endl;
  std::cout << arrSplice->indexOf($Node::make((*arrSplice)[0]->name)) << std::endl;

  auto arrReverse = arrSplice->reverse();
  std::cout << "arrReverse = " << arrReverse->toString() << std::endl;

  auto arrSort = arrConcat->sort([](const $Node &a, const $Node &b) { return a->name > b->name; });
  std::cout << "arrSort = " << arrSort->toString() << std::endl;
  std::cout << ((*arrConcat) == arrSort ? "true" : "false") << std::endl;

  arrSort->forEach([](const $Node &a, std::size_t) { std::cout << a->name << std::endl; });

  auto arrMap = arrSort->map<std::string>([](const $Node &a, std::size_t) {
    std::string upper = a->name;
    std::transform(a->name.begin(), a->name.end(), upper.begin(), [](unsigned char c) { return std::toupper(c); });
    return upper;
  });
  std::cout << "arrMap = " << arrMap->toString() << std::endl;

  auto arrFilter = arrSort->filter([](const $Node &a, std::size_t) { return a->name[0] != 'd'; });
  std::cout << "arrFilter = " << arrFilter->toString() << std::endl;

  auto arrFind = arrFilter->find([](const $Node &a, std::size_t) { return a->name[0] == 'f'; });
  std::cout << "arrFind = " << arrFind->toString() << std::endl;
  auto arrNotFind = arrFilter->find([](const $Node &a, std::size_t) { return a->name[0] == 'd'; });
  std::cout << "arrNotFind = " << (arrNotFind ? arrNotFind->toString() : "null") << std::endl;

  std::cout << arrFilter->findIndex([](const $Node &a, std::size_t) { return a->name[0] == 'f'; }) << std::endl;
  std::cout << arrFilter->findIndex([](const $Node &a, std::size_t) { return a->name[0] == 'd'; }) << std::endl;

  auto sum = arrMap->reduce(([](const std::string &a, const std::string &b, std::size_t) { return a + b; }));
  std::cout << "sum = " << sum << std::endl;

  auto some = arrSort->some([](const $Node &a, std::size_t) { return a->name[0] == 'f'; });
  std::cout << "some = " << (some ? "true" : "false") << std::endl;
  auto every = arrSort->every([](const $Node &a, std::size_t) { return a->name[0] > 'a'; });
  std::cout << "every = " << (every ? "true" : "false") << std::endl;

  auto arrFill = arrMap->fill("a", 2, -2);
  std::cout << "arrFill = " << arrFill->toString() << std::endl;
  std::cout << arrFill->join("+") << std::endl;

  auto arrCopyWithin = arrFill->copyWithin(2, 0, 2);
  std::cout << "arrCopyWithin = " << arrCopyWithin->toString() << std::endl;

  std::cout << "toReversed = " << arrCopyWithin->toReversed()->toString() << std::endl;
  std::cout << arrCopyWithin->toString() << std::endl;

  std::cout << "toSorted = " << arrCopyWithin->toSorted()->toString() << std::endl;
  std::cout << arrCopyWithin->toString() << std::endl;

  std::cout << "toSpliced = " << arrCopyWithin->toSpliced(2, 4)->toString() << std::endl;
  std::cout << arrCopyWithin->toString() << std::endl;

  std::cout << "with = " << arrCopyWithin->with(-2, "x")->toString() << std::endl;
  std::cout << arrCopyWithin->toString() << std::endl;

  auto arrA = $Array<int>::make(std::initializer_list<int>{1, 2, 3});
  auto arrB = $Array<int>::make(std::initializer_list<int>{1, 2, 3});
  std::cout << "arrA = " << arrA->toString() << std::endl;
  std::cout << "arrB = " << arrB->toString() << std::endl;
  std::cout << "arrA == arrB = " << (arrA == arrB ? "true" : "false") << std::endl;
  std::cout << "arrA != arrB = " << (arrA != arrB ? "true" : "false") << std::endl;
  std::cout << "arrA === arrB = " << ((*arrA) == arrB ? "true" : "false") << std::endl;
  std::cout << "arrA !== arrB = " << ((*arrA) != arrB ? "true" : "false") << std::endl;

  return 0;
}