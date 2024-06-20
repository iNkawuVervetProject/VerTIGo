import functools
import io
import unittest

import structlog


def _intercept_test_case(cls):
    if hasattr(cls, "setUp"):
        orig_setUp = cls.setUp

        @functools.wraps(orig_setUp)
        def wrappedSetUp(self):
            self.__logs = io.StringIO()
            structlog.configure(
                logger_factory=structlog.PrintLoggerFactory(file=self.__logs)
            )
            orig_setUp(self)

        setattr(cls, "setUp", wrappedSetUp)

    else:

        def setUp(self):
            self.__logs = io.StringIO()
            structlog.configure(
                logger_factory=structlog.PrintLoggerFactory(file=self.__logs)
            )

        setattr(cls, "setUp", setUp)

    if hasattr(cls, "tearDownClass"):
        orig_tearDownClass = cls.tearDownClass.__func__

        @functools.wraps(orig_tearDownClass)
        @classmethod
        def wrappedTearDownClass(cls):
            structlog.reset_defaults()
            orig_tearDownClass(cls)

        setattr(cls, "tearDownClass", wrappedTearDownClass)
    else:

        @classmethod
        def tearDownClass(cls):
            structlog.reset_defaults()

        setattr(cls, "tearDownClass", tearDownClass)


def _intercept_async_test_case(cls):
    orig_asyncSetUp = cls.asyncSetUp

    async def wrappedSetUp(self):
        self.__logs = io.StringIO()
        structlog.configure(
            logger_factory=structlog.PrintLoggerFactory(file=self.__logs)
        )
        if orig_asyncSetUp is not None:
            await orig_asyncSetUp(self)

    setattr(cls, "asyncSetUp", wrappedSetUp)

    orig_tearDownClass = cls.tearDownClass

    @classmethod
    def wrappedTearDownClass(cls):
        structlog.reset_defaults()
        if orig_tearDownClass is not None:
            orig_tearDownClass(cls)

    setattr(cls, "tearDownClass", wrappedTearDownClass)


def intercept_structlog(cls):
    if issubclass(cls, unittest.TestCase):
        _intercept_test_case(cls)
    elif issubclass(cls, unittest.IsolatedAsyncioTestCase):
        _intercept_async_test_case(cls)
    else:
        raise RuntimeError("could not intercept structlog")
