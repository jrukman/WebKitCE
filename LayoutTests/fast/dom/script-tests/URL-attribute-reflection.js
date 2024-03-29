description("Test reflecting URL attributes with empty string values.");

function testURLReflection(attributeName, tag, scriptAttributeName)
{
    if (!scriptAttributeName)
        scriptAttributeName = attributeName.toLowerCase();

    var element;
    if (tag === "html")
        element = document.documentElement;
    else {
        element = document.createElement(tag);
        document.body.appendChild(element);
    }
    element.setAttribute(scriptAttributeName, "x");
    var xValue = element[attributeName];
    element.setAttribute(scriptAttributeName, "");
    var emptyValue = element[attributeName];
    if (tag !== "html")
        document.body.removeChild(element);

    if (xValue === undefined)
        return "none";
    if (xValue === "x")
        return "non-URL";
    if (xValue !== document.baseURI.replace(/[^\/]+$/, "x"))
        return "error: " + xValue;
    if (emptyValue === "")
        return "non-empty URL";
    if (emptyValue === document.baseURI)
        return "URL";
    return "error: " + value;
}

shouldBe("testURLReflection('attribute', 'element')", "'none'");
shouldBe("testURLReflection('id', 'element')", "'non-URL'");

// The following list comes from the HTML5 document’s attributes index.
// The expected results are based on what the table there says.

shouldBe("testURLReflection('action', 'form')", "'URL'");
shouldBe("testURLReflection('cite', 'blockquote')", "'URL'");
shouldBe("testURLReflection('cite', 'del')", "'URL'");
shouldBe("testURLReflection('cite', 'ins')", "'URL'");
shouldBe("testURLReflection('cite', 'q')", "'URL'");
shouldBe("testURLReflection('data', 'object')", "'non-empty URL'");
shouldBe("testURLReflection('formaction', 'button')", "'URL'");
shouldBe("testURLReflection('formaction', 'input')", "'URL'");
shouldBe("testURLReflection('href', 'a')", "'URL'");
shouldBe("testURLReflection('href', 'area')", "'URL'");
shouldBe("testURLReflection('href', 'link')", "'non-empty URL'");
shouldBe("testURLReflection('href', 'base')", "'URL'");
shouldBe("testURLReflection('icon', 'command')", "'non-empty URL'");
shouldBe("testURLReflection('manifest', 'html')", "'non-empty URL'");
shouldBe("testURLReflection('poster', 'video')", "'non-empty URL'");
shouldBe("testURLReflection('src', 'audio')", "'non-empty URL'");
shouldBe("testURLReflection('src', 'embed')", "'non-empty URL'");
shouldBe("testURLReflection('src', 'iframe')", "'non-empty URL'");
shouldBe("testURLReflection('src', 'img')", "'non-empty URL'");
shouldBe("testURLReflection('src', 'input')", "'non-empty URL'");
shouldBe("testURLReflection('src', 'script')", "'non-empty URL'");
shouldBe("testURLReflection('src', 'source')", "'non-empty URL'");
shouldBe("testURLReflection('src', 'video')", "'non-empty URL'");

// Other reflected URL attributes.

shouldBe("testURLReflection('longDesc', 'img')", "'non-empty URL'");
shouldBe("testURLReflection('lowsrc', 'img')", "'non-empty URL'");

var successfullyParsed = true;
