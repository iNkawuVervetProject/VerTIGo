import inspect
import io
import os
import unittest

import structlog


def _add_logs(obj):
    obj.__logs = io.StringIO()
    if "VERBOSE" not in os.environ:
        structlog.configure(
            logger_factory=structlog.PrintLoggerFactory(file=obj.__logs)
        )


def _teardown():
    structlog.reset_defaults()


def _intercept_test_case(cls):
    if hasattr(cls, "setUp"):
        orig_setUp = cls.setUp

        def wrappedSetUp(self):
            _add_logs(self)
            orig_setUp(self)

        setattr(cls, "setUp", wrappedSetUp)

    else:

        def setUp(self):
            _add_logs(self)

        setattr(cls, "setUp", setUp)

    if hasattr(cls, "tearDownClass"):
        orig_tearDownClass = cls.tearDownClass.__func__

        @classmethod
        def wrappedTearDownClass(cls):
            _teardown()
            orig_tearDownClass(cls)

        setattr(cls, "tearDownClass", wrappedTearDownClass)
    else:

        @classmethod
        def tearDownClass(cls):
            _teardown()

        setattr(cls, "tearDownClass", tearDownClass)

    return cls


def _intercept_async_test_case(cls):
    if hasattr(cls, "asyncSetUp"):
        orig_asyncSetUp = cls.asyncSetUp

        async def wrappedSetUp(self):
            _add_logs(self)
            await orig_asyncSetUp(self)

        setattr(cls, "asyncSetUp", wrappedSetUp)

    else:

        async def setUp(self):
            _add_logs(self)

        setattr(cls, "asyncSetUp", setUp)

    if hasattr(cls, "tearDownClass"):
        orig_tearDownClass = cls.tearDownClass

        @classmethod
        def wrappedTearDownClass(cls):
            _teardown()
            orig_tearDownClass(cls)

        setattr(cls, "tearDownClass", wrappedTearDownClass)
    else:

        @classmethod
        def tearDownClass(cls):
            _teardown()

        setattr(cls, "tearDownClass", tearDownClass)

    return cls


def intercept_structlog(cls):
    if issubclass(cls, unittest.TestCase):
        _intercept_test_case(cls)
    elif issubclass(cls, unittest.IsolatedAsyncioTestCase):
        _intercept_async_test_case(cls)
    else:
        raise RuntimeError("could not intercept structlog")

    return cls
