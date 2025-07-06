#include "gtest/gtest.h"
#include "ngx_mem_pool.h"

// Test fixture for NgxMemoryPool to reuse pool creation/destruction
class PoolTest : public ::testing::Test {
 protected:
  void SetUp() override {
    pool.Create(1024);  // Create a small pool for testing
  }

  void TearDown() override { pool.Destory(); }

  NgxMemoryPool pool;
};

TEST_F(PoolTest, CanCreateAndDestroy) {
  // SetUp and TearDown already create and destroy.
  // This test just checks that it doesn't crash.
  SUCCEED();
}

TEST_F(PoolTest, SmallAllocation) {
  void* p = pool.Alloc(128);
  ASSERT_NE(p, nullptr);
}

TEST_F(PoolTest, LargeAllocation) {
  void* p = pool.Alloc(2048);  // Larger than the initial pool size
  ASSERT_NE(p, nullptr);
}

TEST_F(PoolTest, AllocationsDoNotOverlap) {
  void* p1 = pool.Alloc(64);
  void* p2 = pool.Alloc(64);
  ASSERT_NE(p1, nullptr);
  ASSERT_NE(p2, nullptr);
  // Check that the pointers are separated by at least the requested size
  ptrdiff_t diff = static_cast<char*>(p2) - static_cast<char*>(p1);
  ASSERT_GE(diff, 64);
}

TEST_F(PoolTest, PoolExpansion) {
  // Allocate until the initial 1024-byte block is certainly full
  void* p1 = pool.Alloc(500);
  ASSERT_NE(p1, nullptr);
  void* p2 = pool.Alloc(500);
  ASSERT_NE(p2, nullptr);
  // The second allocation should have triggered a new block creation
}

TEST_F(PoolTest, ResetPool) {
  void* p1_before = pool.Alloc(100);
  pool.Reset();
  void* p1_after = pool.Alloc(100);

  // After reset, the pool should start allocating from the beginning again.
  // The pointers should be the same.
  ASSERT_EQ(p1_before, p1_after);
}

TEST_F(PoolTest, CleanupHandler) {
  bool handler_called = false;
  // A lambda to be used as a cleanup handler
  auto cleanup_func = [](void* data) { *static_cast<bool*>(data) = true; };

  NgxCleanUp* c = pool.AddCleanup(0);
  c->handler = cleanup_func;
  c->data = &handler_called;

  // Destroy the pool, which should trigger the handler
  // We call TearDown manually here to check the side effect before the test
  // ends
  TearDown();

  ASSERT_TRUE(handler_called);

  // Re-create the pool so the automatic TearDown doesn't crash
  SetUp();
}