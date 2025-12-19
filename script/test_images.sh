#!/bin/bash
set -e

VERSION=${1:-"dev"}
FAILED=0

echo "=========================================="
echo "Testing blah2 stack version: ${VERSION}"
echo "=========================================="

# Test 1: blah2 unit tests
echo ""
echo "Running blah2 unit tests..."
if docker run --rm --platform linux/arm64 blah2:${VERSION} sh -c '
  cd /opt/blah2/bin/test/unit 2>/dev/null || { echo "No unit tests directory found"; exit 1; }
  TEST_COUNT=0
  for test in *; do
    if [ -x "$test" ] && [ -f "$test" ]; then
      echo "Running: $test"
      ./"$test" || exit 1
      TEST_COUNT=$((TEST_COUNT + 1))
    fi
  done
  if [ $TEST_COUNT -eq 0 ]; then
    echo "No unit tests found"
    exit 1
  fi
'; then
  echo "Unit tests passed"
else
  echo "Unit tests failed"
  FAILED=$((FAILED + 1))
fi

# Test 2: blah2 functional tests (when they exist)
echo ""
echo "Running blah2 functional tests..."
if docker run --rm --platform linux/arm64 blah2:${VERSION} sh -c '
  if [ -d /opt/blah2/bin/test/functional ]; then
    cd /opt/blah2/bin/test/functional
    TEST_COUNT=0
    for test in *; do
      if [ -x "$test" ] && [ -f "$test" ]; then
        echo "Running: $test"
        ./"$test" || exit 1
        TEST_COUNT=$((TEST_COUNT + 1))
      fi
    done
    if [ $TEST_COUNT -eq 0 ]; then
      echo "No functional tests found"
    fi
  else
    echo "No functional tests found"
  fi
'; then
  echo "Functional tests passed or not found"
else
  echo "Functional tests failed"
  FAILED=$((FAILED + 1))
fi

# Test 3: Check image sizes
echo ""
echo "Checking image sizes..."
TOTAL_SIZE=0
for img in blah2 blah2-api blah2-web blah2-host; do
  if docker image inspect ${img}:${VERSION} > /dev/null 2>&1; then
    SIZE=$(docker image inspect ${img}:${VERSION} --format='{{.Size}}' | awk '{printf "%.0f", $1/1024/1024}')
    echo "  ${img}: ${SIZE}MB"
    TOTAL_SIZE=$((TOTAL_SIZE + SIZE))
  else
    echo "  ${img}: Image not found"
    FAILED=$((FAILED + 1))
  fi
done

echo "  Total: ${TOTAL_SIZE}MB"
if [ $TOTAL_SIZE -gt 4096 ]; then
  echo "FAILED: Total image size (${TOTAL_SIZE}MB) exceeds 4GB limit"
  FAILED=$((FAILED + 1))
fi

# Test 4: Container startup checks
echo ""
echo "Testing container startup..."

# Test blah2-api 
echo "Testing blah2-api..."
if [ -f "config/config.yml" ]; then
  if docker run --rm -d --name test-blah2-api-$$ --platform linux/arm64 \
    -v $(pwd)/config:/usr/src/app/config \
    blah2-api:${VERSION} node server.js /usr/src/app/config/config.yml > /dev/null 2>&1; then
    sleep 5
    if docker ps | grep -q test-blah2-api-$$; then
      echo "blah2-api starts successfully"
      docker stop test-blah2-api-$$ > /dev/null 2>&1
    else
      echo "blah2-api failed to stay running"
      docker logs test-blah2-api-$$
      FAILED=$((FAILED + 1))
    fi
  else
    echo "blah2-api failed to start"
    FAILED=$((FAILED + 1))
  fi
  docker rm -f test-blah2-api-$$ > /dev/null 2>&1 || true
else
  echo "WARNING: config/config.yml not found, skipping blah2-api test"
fi

# Test blah2-web and blah2-host
for img in blah2-web blah2-host; do
  echo "Testing ${img}..."
  if docker run --rm -d --name test-${img}-$$ --platform linux/arm64 ${img}:${VERSION} > /dev/null 2>&1; then
    sleep 3
    if docker ps | grep -q test-${img}-$$; then
      echo "${img} starts successfully"
      docker stop test-${img}-$$ > /dev/null 2>&1
    else
      echo "${img} failed to stay running"
      docker logs test-${img}-$$
      FAILED=$((FAILED + 1))
    fi
  else
    echo "${img} failed to start"
    FAILED=$((FAILED + 1))
  fi
  docker rm -f test-${img}-$$ > /dev/null 2>&1 || true
done

# Summary
echo ""
echo "=========================================="
if [ $FAILED -eq 0 ]; then
  echo "All tests passed"
  echo "=========================================="
  exit 0
else
  echo "$FAILED test suite(s) failed"
  echo "=========================================="
  exit 1
fi