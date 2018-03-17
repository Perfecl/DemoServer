#ifndef __TRIE_TREE_H__
#define __TRIE_TREE_H__
#include <locale.h>
#include <wchar.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <string>
#include <boost/shared_ptr.hpp>

class TrieTreeNode {
 public:
  TrieTreeNode(wchar_t inputChar, int capcity = 8)
      : m_CurrentChar(inputChar), m_IsLeafNode(false) {
    this->m_SubNodes.reset(new std::vector<TrieTreeNode>);
    this->m_SubNodes->reserve(capcity);
  }

  TrieTreeNode &AddChar(wchar_t inputChar, bool finished) {
    TrieTreeNode inputNode(inputChar);
    std::vector<TrieTreeNode>::iterator iter =
        std::lower_bound(m_SubNodes->begin(), m_SubNodes->end(), inputNode);
    if (iter == m_SubNodes->end() || iter->m_CurrentChar != inputChar) {
      m_SubNodes->insert(iter, inputNode);
      iter = std::lower_bound(m_SubNodes->begin(), m_SubNodes->end(), inputNode);
    }
    if (finished) {
      iter->m_IsLeafNode = true;
    }
    return *iter;
  }

  TrieTreeNode *GetNodeByChar(wchar_t inputChar) {
    TrieTreeNode inputNode(inputChar);
    std::vector<TrieTreeNode>::iterator iter =
        std::lower_bound(m_SubNodes->begin(), m_SubNodes->end(), inputNode);
    if (iter == m_SubNodes->end() || iter->m_CurrentChar != inputChar) {
      return NULL;
    }
    return &*iter;
  }

  bool operator==(const TrieTreeNode &node) const {
    return this->m_CurrentChar == node.m_CurrentChar;
  }
  bool operator<(const TrieTreeNode &node) const {
    return this->m_CurrentChar < node.m_CurrentChar;
  }

  bool IsLeafNode() const { return m_IsLeafNode; }
  void Reset() {
    this->m_SubNodes->clear();
    this->m_CurrentChar = wchar_t();
  }

 private:
  friend class TrieTree;
  wchar_t m_CurrentChar;
  bool m_IsLeafNode;
  boost::shared_ptr<std::vector<TrieTreeNode> > m_SubNodes;
};

class TrieTree {
 public:
  TrieTree() : root(0) {
    setlocale(LC_ALL, "en_US.utf8");
  }

  void AddMatchString(const std::string& str) {
    size_t str_len = str.length();
    wchar_t wstr[str_len + 1];
    size_t wstr_len = str_len;
    wstr[wstr_len] = 0;

    ConvertToWString(str.c_str(), str_len, wstr, wstr_len);
    std::transform(wstr, wstr + wstr_len, wstr, ::tolower);
    AddMatchString(wstr, wstr_len);
  }

  void Clear() { root.Reset(); }

  bool Match(const std::string& str) {
    size_t str_len = str.length();
    wchar_t wstr[str_len + 1];
    size_t wstr_len = str_len;
    wstr[wstr_len] = 0;

    ConvertToWString(str.c_str(), str_len, wstr, wstr_len);
    if (wstr_len > str_len) {
      return true;
    }

    std::transform(wstr, wstr + wstr_len, wstr, ::tolower);
    bool result = MatchString(wstr, wstr_len, &TrieTree::Broken, L'*', wstr);
    return result;
  }

  bool Transform(std::string& str, wchar_t mask = L'*') {
    size_t str_len = str.length();
    wchar_t wstr[str_len + 1];
    size_t wstr_len = str_len;
    wstr[wstr_len] = 0;

    ConvertToWString(str.c_str(), str_len, wstr, wstr_len);
    if (wstr_len > str_len) {
      return true;
    }
    wchar_t wstr_old[str_len + 1];
    wcscpy(wstr_old, wstr);

    std::transform(wstr, wstr + wstr_len, wstr, ::tolower);
    bool result =
        MatchString(wstr, wstr_len, &TrieTree::ReplaceChar, mask, wstr_old);
    if (result) {
      ConvertToCString(wstr_old, wcslen(wstr_old), str, str_len);
    }

    return result;
  }

 private:
  typedef bool (TrieTree::*CallbackPtr)(wchar_t *, size_t, wchar_t);

  void AddMatchString(const wchar_t *str, size_t len) {
    TrieTreeNode *node = &root;
    for (size_t i = 0; i < len; ++i) {
      node = &node->AddChar(str[i], i == (len - 1));
    }
  }

  int32_t MatchString(wchar_t *wstr, size_t wstr_len, CallbackPtr ptr,
                      wchar_t mask, wchar_t *wstr_old) {
    int32_t matchCount = 0;
    for (size_t i = 0; i < wstr_len; ++i) {
      int32_t depth = 0;
      wchar_t *wstr_begin = wstr + i;
      TrieTreeNode *node = &root;
      while (node != NULL) {
        node = node->GetNodeByChar(wstr_begin[depth]);
        depth = node ? depth + 1 : 0;
        if (node != NULL && node->IsLeafNode()) {
          ++matchCount;
          bool isBreak =
              (this->*ptr)(wstr_old + (wstr_begin - wstr), depth, mask);
          if (isBreak) {
            return matchCount;
          }
        }
      }
    }
    return matchCount;
  }

  bool ReplaceChar(wchar_t *wstr, size_t wstr_len, wchar_t mask) {
    for (size_t i = 0; i < wstr_len; ++i) {
      wstr[i] = mask;
    }
    return false;
  }

  bool Broken(wchar_t *wstr, size_t wstr_len, wchar_t mask) { return false; }

  static void ConvertToWString(const char *str, size_t str_len, wchar_t *wstr,
                               size_t &wstr_len) {
    wstr_len = mbstowcs(wstr, str, wstr_len);
  }
  static void ConvertToCString(const wchar_t *wstr, size_t wstr_len, std::string& str,
                               size_t &str_len) {
    str_len = wcstombs(const_cast<char*>(str.c_str()), wstr, str_len);
    str.resize(str_len);
  }

 private:
  TrieTreeNode root;
};

#endif
