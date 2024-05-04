# blah2 Test

A set of tests are provided for development/debugging.

## Framework

The test framework is [catch2](https://github.com/catchorg/Catch2).

## Types

The test files are split across directories defined by the type of test.

- **Unit tests** will test the class in isolation. The directory structure mirrors *src*.
- **Functional tests** will test that expected outputs are achieved from defined inputs. An example would be checking the program turns a specific IQ data set to a specific delay-Doppler map. This test category will rely on golden data.
- **Comparison tests** will compare different methods of performing the same task. An example would be comparing 2 methods of clutter filtering. Metrics to be compared may include time and performance. Note there is no specific pass/fail criteria for comparison tests - this is purely for information. A comparison test will pass if executed successfully. Any comparison testing on input parameters for a single class will be handled in the unit test.

## Usage

All tests are compiled when building, however tests be run manually.

- Run a single unit test for "TestClass".

```
sudo docker exec -it blah2 /blah2/bin/test/unit/testClass
```

- Run a single functional test for "TestFunctional".

```
sudo docker exec -it blah2 /blah2/bin/test/functional/testFunctional
```

- Run a single comparison test for "TestComparison".

```
sudo docker exec -it blah2 /blah2/bin/test/comparison/testComparison
```

- *TODO:* Run all test cases.

```
sudo docker exec -it blah2 /blah2/bin/test/runall.sh
sudo docker exec -it blah2 /blah2/bin/test/unit/runall.sh
sudo docker exec -it blah2 /blah2/bin/test/functional/runall.sh
sudo docker exec -it blah2 /blah2/bin/test/comparison/runall.sh
```