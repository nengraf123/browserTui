#!/bin/sh
URL="$1"

node -e '
const puppeteer = require("puppeteer");

(async () => {
  const browser = await puppeteer.launch({headless: "new"});
  const page = await browser.newPage();
  await page.goto(process.argv[1], {waitUntil: "networkidle2"});

  const elements = await page.evaluate(() => {
    return [...document.querySelectorAll("*")].map(el => {
      const r = el.getBoundingClientRect();
      if (r.width === 0 || r.height === 0) return null;
      const style = getComputedStyle(el);
      if (style.display === "none" || style.visibility === "hidden") return null;
      return {
        tag: el.tagName,
        x: Math.round(r.x),
        y: Math.round(r.y),
        w: Math.round(r.width),
        h: Math.round(r.height),
        src: el.src || null,
        href: el.href || null,
        text: el.children.length === 0 ? el.textContent.trim().slice(0,200) : null
      };
    }).filter(Boolean);
  });

  console.log(JSON.stringify(elements));
  await browser.close();
})();
' "$URL"
