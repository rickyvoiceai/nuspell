#include "compound_corrector.hxx"

#include <cassert>
#include <iostream>
#include <string>

using namespace std;

int main() {
	cout << "[Nuspell C++14 API Smoke Test]"<<endl;

	// 1. Construction
	CompoundCorrector cc("res/en_US.aff");
	cout << "Constructed OK"<<endl;

	// 2. Simple merge: acronym
	assert(cc.Correct("ap i") == "API");
	cout << "Merge: 'ap i' -> 'API' OK"<<endl;

	// 3. Simple merge: compound
	assert(cc.Correct("tachy cardia") == "tachycardia");
	cout << "Merge: 'tachy cardia' -> 'tachycardia' OK"<<endl;

	// 4. Guard: stop-word pair should NOT merge
	assert(cc.Correct("be at") == "be at");
	cout << "Guard: 'be at' unchanged OK"<<endl;

	// 5. Guard: another stop-word pair
	assert(cc.Correct("in to") == "in to");
	cout << "Guard: 'in to' unchanged OK"<<endl;

	// 6. Single-token should pass through
	assert(cc.Correct("hello") == "hello");
	cout << "Pass-through: 'hello' OK"<<endl;

	// 7. Numbers should pass through
	assert(cc.Correct("123") == "123");
	cout << "Pass-through: '123' OK"<<endl;

	// 8. Chinese CJK should pass through
	assert(cc.Correct("中文") == "中文");
	cout << "Pass-through: '中文' OK"<<endl;

	// 9. Single-word correction disabled by default
	assert(cc.Correct("fighe") == "fighe");
	cout << "fix_single=false: 'fighe' unchanged OK"<<endl;

	// 10. Single-word correction enabled
	assert(cc.Correct("fighe", true) == "fight");
	cout << "fix_single=true: 'fighe' -> 'fight' OK"<<endl;

	// 11. Ambiguous merge: "a" is a short valid, "ce" + "o" merges
	assert(cc.Correct("a ce o") == "a CEO");
	cout << "Merge: 'a ce o' -> 'a CEO' OK"<<endl;

	// 12. Full sentence test
	assert(cc.Correct("she is a ce o") == "she is a CEO");
	cout << "Sentence: 'she is a ce o' -> 'she is a CEO' OK"<<endl;

	// 13. CorrectWithStatus — merge produces correct status
	auto r = cc.CorrectWithStatus("ap i");
	assert(r.text == "API");
	assert(r.status.size() == 2);
	assert(r.status[0] == 1);
	assert(r.status[1] == -1);
	cout << "Status: [1,-1] for merge OK"<<endl;

	// 14. CorrectWithStatus — single-word fix status
	auto r2 = cc.CorrectWithStatus("fighe", true);
	assert(r2.text == "fight");
	assert(r2.status.size() == 1);
	assert(r2.status[0] == 2);
	cout << "Status: [2] for single-word fix OK"<<endl;

	// 15. Bundle mode — verify it also works
	// (Bundle loaded in separate test via test_compound -b)

	cout << endl << "All smoke tests passed!"<<endl;
	return 0;
}
