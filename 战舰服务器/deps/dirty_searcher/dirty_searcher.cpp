#include "trie_tree.h"

static TrieTree kDirtyWords;

void AddDirtyWord(std::string& str) {
  if (str.empty()) return;
  if (*(str.end() - 1) == '\n') str.resize(str.length() - 1);
  if (*(str.end() - 1) == '\r') str.resize(str.length() - 1);
  kDirtyWords.AddMatchString(str);
}

void ReplaceDirtyWords(std::string& str) { kDirtyWords.Transform(str); }

bool ContainsDirtyWords(const std::string& str) {
  return kDirtyWords.Match(str);
}
