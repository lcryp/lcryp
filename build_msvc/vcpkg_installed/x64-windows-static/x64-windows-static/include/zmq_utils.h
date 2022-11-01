#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)               \
  || defined(_MSC_VER)
#if defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wcpp"
#pragma GCC diagnostic ignored "-Werror"
#pragma GCC diagnostic ignored "-Wall"
#endif
#pragma message(                                                               \
  "Warning: zmq_utils.h is deprecated. All its functionality is provided by zmq.h.")
#if defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
#endif
