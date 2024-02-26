# Slic3r3

## Structure

1. Domain: Contains _a domain logic_ mainly data model entities encapsulating backend ones (Model, DynamicPrintConfig, Print, etc.)
2. Buiseness: Contains _a buisenss logic_ implementing use-cases in form of Interactor and Worker (if needed), using domain logic layer.
3. App: Contains _an application / presentation logic_ implementing presentation layer (views, scene-graph) using business logic layer.

### Dependency flow

Following figure depicts only allowed dependencies among layers:

```
+----------+     +------------+       +------+
| Domain   |<----| Buiseness  |<------|      |
|          |     +------------+       | App  |
|          |<-----(discouraged)-------|      |
+----------+                          +------+
```

In case oposite dependency flow is needed,  
[dependency inversion principle](https://en.wikipedia.org/wiki/Dependency_inversion_principle#Implementations)
should be applied.

Example: PrintConfigInteractor and confirmation dialogs.

```

namespace Slic3r::Biz {
	struct ConfigInteractorInputPort {
		virtual bool presentConfirmDialog(...) = 0;
	};

	class ConfigInteractor {
		void setConfigurationChange(...);
	private:
		ConfigInteractorInputPort* m_input;	
	};
}

namespace Slic3r::App {
	class ConfigPresenter : public Slic3r::Biz::ConfigInteractorInputPort {
	public:
	    bool presentConfirmDialog(...) override;
	};
}


```
