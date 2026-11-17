#include "region_config.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <unistd.h>

TEST(RegionConfig, ParsesRegion)
{
    const auto r = app::parseRegion("[0,0] [500,100]");
    ASSERT_TRUE(r.has_value()) << r.error();
    EXPECT_EQ(r->left, 0);
    EXPECT_EQ(r->top, 0);
    EXPECT_EQ(r->width, 500);
    EXPECT_EQ(r->height, 100);
}

TEST(RegionConfig, ToleratesSpaces)
{
    const auto r = app::parseRegion("  [ 12 , 34 ]   [ 500 ,100 ] ");
    ASSERT_TRUE(r.has_value()) << r.error();
    EXPECT_EQ(r->left, 12);
    EXPECT_EQ(r->top, 34);
    EXPECT_EQ(r->width, 500);
    EXPECT_EQ(r->height, 100);
}

TEST(RegionConfig, RejectsMalformedRegions)
{
    EXPECT_FALSE(app::parseRegion("").has_value());
    EXPECT_FALSE(app::parseRegion("[0,0]").has_value());
    EXPECT_FALSE(app::parseRegion("0,0 500,100").has_value());
    EXPECT_FALSE(app::parseRegion("[0,0] [500]").has_value());
    EXPECT_FALSE(app::parseRegion("[-1,0] [500,100]").has_value());
    EXPECT_FALSE(app::parseRegion("[0,0] [0,100]").has_value());
    EXPECT_FALSE(app::parseRegion("[0,0] [500,100] [1,1]").has_value());
}

TEST(RegionConfig, ParsesFileWithCommentsAndCrlf)
{
    const auto c = app::parseRegionConfig("# regions\r\n\r\nGAME_LIST_REGION = [0,0] [500,100]\r\n"
                                          "OTHER=[10,20] [30,40]\r\n");
    ASSERT_TRUE(c.has_value()) << c.error();
    ASSERT_EQ(c->size(), 2u);
    EXPECT_EQ(c->at("GAME_LIST_REGION").width, 500);
    EXPECT_EQ(c->at("OTHER").top, 20);
}

TEST(RegionConfig, ReportsLineOfBadEntries)
{
    const auto bad = app::parseRegionConfig("A = [0,0] [1,1]\nGAME_LIST_REGION = [0,0]");
    ASSERT_FALSE(bad.has_value());
    EXPECT_NE(bad.error().find("line 2"), std::string::npos);

    EXPECT_FALSE(app::parseRegionConfig("= [0,0] [1,1]").has_value());
    EXPECT_FALSE(app::parseRegionConfig("[0,0] [1,1]").has_value());
    EXPECT_FALSE(app::parseRegionConfig("A = [0,0] [1,1]\nA = [0,0] [2,2]").has_value());
}

TEST(RegionConfig, RequireRegionChecksPresenceAndWindowBounds)
{
    const auto c = app::parseRegionConfig("GAME_LIST_REGION = [600,700] [424,68]\nBIG = [600,0] [500,10]");
    ASSERT_TRUE(c.has_value()) << c.error();
    EXPECT_TRUE(app::requireRegion(*c, app::kGameListRegion, 1024, 768).has_value());
    EXPECT_FALSE(app::requireRegion(*c, "BIG", 1024, 768).has_value());
    EXPECT_FALSE(app::requireRegion(*c, "MISSING", 1024, 768).has_value());
}

TEST(RegionConfig, LoadsFileAndPathFromEnvironment)
{
    const auto file = std::filesystem::temp_directory_path()
        / ("the_app_AI_regions_" + std::to_string(::getpid()) + ".txt");
    {
        std::ofstream out(file);
        out << "GAME_LIST_REGION = [0,0] [500,100]\n";
    }
    const auto c = app::loadRegionConfig(file);
    std::filesystem::remove(file);
    ASSERT_TRUE(c.has_value()) << c.error();
    EXPECT_EQ(c->at("GAME_LIST_REGION").height, 100);
    EXPECT_FALSE(app::loadRegionConfig("/nonexistent/region_config.txt").has_value());

    ::unsetenv("THE_APP_AI_REGION_CONFIG");
    EXPECT_EQ(app::regionConfigPath(), "region_config.txt");
    ::setenv("THE_APP_AI_REGION_CONFIG", "/tmp/r.txt", 1);
    EXPECT_EQ(app::regionConfigPath(), "/tmp/r.txt");
    ::unsetenv("THE_APP_AI_REGION_CONFIG");
}
