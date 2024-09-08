#include <Geode/Geode.hpp>
using namespace geode::prelude;

#include <Geode/modify/CCTextInputNode.hpp>
#include <clip.h>

class Intercept : public CCLayer, public CCTextFieldDelegate {
	CCTextInputNode* m_node;
 public:
	bool m_touchBegan;
	std::vector<int> m_selected;

	static Intercept* create(CCTextInputNode* n) {
		auto pRet = new Intercept();
		
		if (pRet && pRet->CCLayer::init()) {
			if (pRet->init(n)) {
				pRet->autorelease();
				return pRet;
			}
		}
		
		CC_SAFE_DELETE(pRet);
		return NULL;
	}

	bool init(CCTextInputNode* n) {
		m_node = n;
		m_touchBegan = false;

		return true;
	}

	void updateSelectVisual() {
		int i = 0;
		for (auto& c : CCArrayExt<CCSprite*>(m_node->m_placeholderLabel->getChildren())) {
			if (std::find(m_selected.begin(), m_selected.end(), i) != m_selected.end()) {
				c->setColor(Mod::get()->getSettingValue<ccColor3B>("color"));
			} else {
				c->setColor(m_node->m_textColor);
			}

			++i;
		}
	}

	void deselect() {
		m_selected.clear();
		updateSelectVisual();
	}

	void copySelection() {
		clip::set_text(std::string(m_node->getString()).substr(m_selected.front(), 1 + m_selected.back() - m_selected.front()));
	}

	void deleteSelection() {
		auto str = std::string(m_node->getString());
		std::sort(m_selected.begin(), m_selected.end());


		auto st = str.erase(m_selected.front(), 1 + m_selected.back() - m_selected.front());
		auto spr = static_cast<CCSprite*>(m_node->m_placeholderLabel->getChildren()->objectAtIndex(m_selected.front()));

		deselect();
		m_node->setString(st);
		m_node->updateCursorPosition(m_node->m_placeholderLabel->convertToWorldSpace(spr->getPosition()) + ccp(-10, 0), m_node->boundingBox());
	}

	bool ccTouchBegan(CCTouch* t, CCEvent* e) override {
		if (!m_selected.empty()) {
			deselect();
			return true;
		} else {
			m_touchBegan = true;
			auto ret = m_node->ccTouchBegan(t, e);
			m_touchBegan = false;

			return ret;
		}
	}

	void ccTouchMoved(CCTouch* t, CCEvent* e) override {
		auto a = t->getStartLocation().x;
		auto b = t->getLocation().x;

		auto start = std::min(a, b);
		auto end = std::max(a, b);

		int i = 0;
		m_selected.clear();

		for (auto& c : CCArrayExt<CCSprite*>(m_node->m_placeholderLabel->getChildren())) {
			auto x = m_node->m_placeholderLabel->convertToWorldSpace(c->getPosition()).x;

			if (x >= start && x <= end) {
				m_selected.push_back(i);
			}
			++i;
		}

		updateSelectVisual();
		m_node->updateCursorPosition(t->getLocation(), m_node->boundingBox());
	}

	bool onTextFieldInsertText(CCTextFieldTTF* sender, char const* text, int len, cocos2d::enumKeyCodes code) override {
		#ifdef GEODE_IS_MACOS
			bool cmd = CCDirector::sharedDirector()->getKeyboardDispatcher()->getCommandKeyPressed();
		#else
			bool cmd = CCDirector::sharedDirector()->getKeyboardDispatcher()->getControlKeyPressed();
		#endif

		if (cmd) {
			switch (code) {
				case 'C':
					if (!m_selected.empty())
						copySelection();
					return true;
				case 'A':
					m_selected.clear();
					for (int i = 0; i < m_node->m_placeholderLabel->getChildrenCount(); ++i) {
						m_selected.push_back(i);
					}
					updateSelectVisual();
					return true;
				case 'X':
					if (!m_selected.empty()) {
						copySelection();
						deleteSelection();
					}
					return true;
					break;
				default: break;
			}
		}

		if (!m_selected.empty()) {
			deleteSelection();
		}
		return m_node->onTextFieldInsertText(sender, text, len, code);
	}
	bool onTextFieldDeleteBackward(CCTextFieldTTF* sender, char const* delText, int len) override {
		if (!m_selected.empty()) {
			deleteSelection();
			return true;
		}
		return m_node->onTextFieldDeleteBackward(sender, delText, len);
	}
	bool onTextFieldAttachWithIME(CCTextFieldTTF* sender) override {
		return m_node->onTextFieldAttachWithIME(sender);
	}
	bool onTextFieldDetachWithIME(CCTextFieldTTF* sender) override {
		return m_node->onTextFieldDetachWithIME(sender);
	}
	void textChanged() override {
		m_node->textChanged();
	}
};

class $modify(CCTextInputNode) {
	struct Fields {
		Intercept* m_intercept;
	};

	void visit() {
		if (m_fields->m_intercept == nullptr && m_placeholderLabel != nullptr) {
			m_fields->m_intercept = Intercept::create(this);
			m_fields->m_intercept->setTouchMode(cocos2d::kCCTouchesOneByOne);
			m_fields->m_intercept->registerWithTouchDispatcher();
			m_fields->m_intercept->setTouchEnabled(true);

			this->addChild(m_fields->m_intercept);
			m_textField->setDelegate(m_fields->m_intercept);
		} else if (m_placeholderLabel == nullptr && m_fields->m_intercept != nullptr) {
			m_fields->m_intercept->removeFromParentAndCleanup(true);
			m_fields->m_intercept = nullptr;
		}

		visit();
	}

	bool ccTouchBegan(CCTouch* t, CCEvent* e) {
		if (!m_fields->m_intercept || m_fields->m_intercept->m_touchBegan)
			return ccTouchBegan(t, nullptr);
		else
			return false;
	}
};
