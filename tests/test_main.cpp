#include "test_runner.h"

#include <iostream>

void testAudioFrame();
void testGain();
void testLimiter();
void testAnalyzer();
void testOscillator();
void testWavWriter();
void testFilters();
void testDelayLine();
void testSafetyGovernor();
void testDspGraph();

int main()
{
    RUN_TEST(testAudioFrame);
    RUN_TEST(testGain);
    RUN_TEST(testLimiter);
    RUN_TEST(testAnalyzer);
    RUN_TEST(testOscillator);
    RUN_TEST(testWavWriter);
    RUN_TEST(testFilters);
    RUN_TEST(testDelayLine);
    RUN_TEST(testSafetyGovernor);
    RUN_TEST(testDspGraph);

    std::cout << "Tests run: " << gTestsRun
              << " Failed: " << gTestsFailed << "\n";

    return gTestsFailed == 0 ? 0 : 1;
}
