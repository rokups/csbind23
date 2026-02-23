#include "csbind23/cabi/converter.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(StringConverterTests, CAbiTypeAndPInvokeNamesAreStable)
{
    EXPECT_EQ(csbind23::cabi::c_abi_type_name_for<std::string>(), "const char*");
    EXPECT_EQ(csbind23::cabi::pinvoke_type_name_for<std::string>(), "string");

    EXPECT_EQ(csbind23::cabi::c_abi_type_name_for<std::string&>(), "void*");
    EXPECT_EQ(csbind23::cabi::pinvoke_type_name_for<std::string&>(), "System.IntPtr");

    EXPECT_EQ(csbind23::cabi::c_abi_type_name_for<const std::string&>(), "const void*");
    EXPECT_EQ(csbind23::cabi::pinvoke_type_name_for<const std::string&>(), "System.IntPtr");
}

TEST(StringConverterTests, ToCAbiUsesStableThreadLocalStorage)
{
    const std::string value = "hello";
    const char* ptr = csbind23::cabi::Converter<std::string>::to_c_abi(value);
    ASSERT_NE(ptr, nullptr);
    EXPECT_STREQ(ptr, "hello");
}

TEST(StringConverterTests, FromCAbiCopiesValue)
{
    const char* text = "copied";
    const std::string converted = csbind23::cabi::Converter<std::string>::from_c_abi(text);
    EXPECT_EQ(converted, "copied");
}
