import re

with open('src/manager/GameManager.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# New method with multi-line dialogues
new_method = r'''void GameManager::applyNpcDialogueTemplates()
{
    // Per-NPC unique dialogues keyed by npc id.
    // Each NPC has 11 entries (rank 0..10). Each entry is a vector of dialogue lines.
    static const std::map<std::string, std::vector<std::vector<std::string>>> kNpcDialogues = {
        // Fool - Yang Ming: a wanderer obsessed with the "perfect taste"
        {"sl_npc_0",
         {
             {"Yang Ming: \"Hey there! You look like someone who appreciates good food. Ever heard of... yellow-braised chicken?\"", "Yang Ming: \"I travel the world searching for flavors that remind me of home. That dish... it was perfection.\"", "Yang Ming: \"You also came from another world? I knew I wasn't the only one! The tarot cards... they hold the key.\""},
             {"Yang Ming: \"Hey, good to see you again! I've been thinking about that chicken you mentioned.\"", "Yang Ming: \"This world has strange ingredients, but nothing quite matches the golden sauce of home.\"", "Yang Ming: \"Every Persona carries a fragment of memory. Mine? It tastes like home.\""},
             {"Yang Ming: \"I met a chef once who could make miracles with mushrooms. Maybe we can find him?\"", "Yang Ming: \"The monsters here drop strange ingredients. I've been collecting them for a 'special recipe.'\"", "Yang Ming: \"You know, this world isn't so bad. At least the monsters don't complain about my cooking.\""},
             {"Yang Ming: \"I've been thinking... maybe the way back is through understanding all 22 Arcana.\"", "Yang Ming: \"Each Arcana represents a piece of the puzzle. Only together can they reveal the path home.\"", "Yang Ming: \"You're the only one who understands the longing for that flavor. Let's find it together.\""},
             {"Yang Ming: \"My Persona gets stronger every time I remember that taste. Memory is power here.\"", "Yang Ming: \"The Fool's journey is one of discovery. And today, I discovered a new recipe!\"", "Yang Ming: \"Whether we find that chicken again or not, I'm glad to have a friend who understands.\""},
             {"Yang Ming: \"I've been experimenting with monster ingredients. Want to try my latest creation?\"", "Yang Ming: \"It's not yellow-braised chicken, but it's got potential. Here, taste this!\"", "Yang Ming: \"Maybe the secret isn't finding the same dish, but creating something new that carries the same warmth.\""},
             {"Yang Ming: \"You know, I've been thinking about our journey. The 22 Arcana, the Personas, this whole world...\"", "Yang Ming: \"What if the reason we're here is because someone wanted us to find something?\"", "Yang Ming: \"Not just a way home, but a reason to stay. A purpose beyond the familiar.\""},
             {"Yang Ming: \"I found something interesting in the forest yesterday. A mushroom that glows golden.\"", "Yang Ming: \"It reminded me of the sauce from that chicken dish. Maybe... maybe it's connected.\"", "Yang Ming: \"The world works in mysterious ways. Even here, the memory of home finds a way to reach us.\""},
             {"Yang Ming: \"I've been talking to the other Arcana holders. They all have their own stories, their own reasons.\"", "Yang Ming: \"Some seek power, some seek knowledge. But you and I? We just want that taste of home.\"", "Yang Ming: \"And maybe that's exactly what makes our bonds the strongest. Shared longing creates the deepest ties.\""},
             {"Yang Ming: \"I've almost perfected the recipe. Just a few more ingredients, and I'll have something truly special.\"", "Yang Ming: \"It's not the same as the original, but it carries the same spirit. The same love.\"", "Yang Ming: \"When I share it with you, I want you to remember: home isn't a place, it's a feeling. And we can carry it anywhere.\""},
             {"Yang Ming: \"I've finally done it. I've recreated the yellow-braised chicken of my dreams!\"", "Yang Ming: \"Come, sit with me. Let's eat together, as friends who found each other across worlds.\"", "Yang Ming: \"No matter where our journey takes us next, we'll always have this moment. This taste. This bond.\""},
         }},
        // Magician - Eric: a scholar studying Personas
        {"sl_npc_1",
         {
             {"Eric: \"Ah, another awakened one. I can sense the resonance of your Persona from here.\"", "Eric: \"The Magician Arcana represents power and resourcefulness. It suits me, don't you think?\"", "Eric: \"I've been cataloguing the 22 Major Arcana Personas. Your collection is... impressive.\""},
             {"Eric: \"Each Persona corresponds to a skill. The Fool starts the journey; the World completes it.\"", "Eric: \"Did you know? The monsters at night carry crystallized memories. I call them 'Star Shards.'\"", "Eric: \"I need more data on combat applications. Would you help me study the monsters?\""},
             {"Eric: \"Your ability to switch Personas mid-battle is fascinating. Most people are bound to one.\"", "Eric: \"The tarot cards aren't just symbols. They're anchors for the soul in this world.\"", "Eric: \"I've made a breakthrough! The Star Shards can temporarily boost Persona abilities.\""},
             {"Eric: \"With your help, my research is almost complete. The truth about this world is within reach.\"", "Eric: \"But truth is like a Persona: the more you understand it, the more complex it becomes.\"", "Eric: \"Thank you, friend. You've helped me understand that knowledge means nothing without bonds.\""},
             {"Eric: \"I've been analyzing the patterns in Persona awakenings. There's a correlation between emotional state and power output.\"", "Eric: \"When you're thinking about that yellow-braised chicken, your Persona resonates at a unique frequency.\"", "Eric: \"Emotion is the key. The stronger the feeling, the stronger the Persona. Your love for that dish... it's your greatest power source.\""},
             {"Eric: \"The Arcana system is more complex than I initially thought. Each card doesn't just grant power—it shapes personality.\"", "Eric: \"The Fool isn't actually foolish. It represents the pure potential of a new beginning. Like you, arriving here with nothing but memories.\"", "Eric: \"And from that emptiness, infinite possibility. That's the true meaning of the Arcana.\""},
             {"Eric: \"I've discovered something troubling. The monsters aren't natural creatures.\"", "Eric: \"They're manifestations of... something. Fears, regrets, lost memories. Fragmented souls, given form.\"", "Eric: \"That explains why they drop crystallized memories. They're literally made of the stuff.\""},
             {"Eric: \"I've been collaborating with the Priestess. Her visions and my data are painting a clearer picture.\"", "Eric: \"This world... it's not just a world. It's a mirror. A reflection of collective human consciousness.\"", "Eric: \"The 22 Arcana represent 22 fundamental aspects of the human psyche. Understanding them means understanding ourselves.\""},
             {"Eric: \"My latest theory: the reason you can carry 10 Personas is because the human psyche has room for 10 archetypes.\"", "Eric: \"But you can only wield one at a time because the mind needs focus. Multiple active Personas would cause... instability.\"", "Eric: \"It's like having 10 recipes but only one kitchen. You can prepare them all, but cook only one at a time.\""},
             {"Eric: \"I've completed my research. The final piece of the puzzle... it's not what I expected.\"", "Eric: \"The 22 Arcana aren't separate entities. They're fragments of a single, greater whole.\"", "Eric: \"And that whole? It's not a Persona. It's a Person. Someone who embodies all 22 aspects simultaneously. Someone like... well, that's for you to discover.\""},
         }},
        // High Priestess - Selena: a mysterious fortune teller
        {"sl_npc_2",
         {
             {"Selena: \"The cards have foretold your arrival. The Fool who seeks the taste of home...\"", "Selena: \"I see visions in the moonlight. The Priestess gazes into the abyss and sees truth.\"", "Selena: \"Your destiny is intertwined with the 22 Arcana. Only by awakening them all will you find your way.\""},
             {"Selena: \"The night brings monsters, but also moonlight mushrooms. They glow with ancient memory.\"", "Selena: \"Beware the Shadow that walks between worlds. It hungers for what you cherish most.\"", "Selena: \"I have seen a vision: you and a plate of golden chicken, separated by a veil of stars.\""},
             {"Selena: \"The moon is full tonight. My visions are clearer than ever. Would you hear your fortune?\"", "Selena: \"Each NPC you meet is a fragment of the world's memory. We are all echoes of the tarot.\"", "Selena: \"Your bond with the Fool grows strong. Together, you may yet pierce the veil between worlds.\""},
             {"Selena: \"I see it now. The yellow-braised chicken is not just food. It is the anchor that calls you home.\"", "Selena: \"The moon reveals what the sun conceals. Tonight, the truth shines bright.\"", "Selena: \"You have grown much since we first met. The cards smile upon you, traveler.\""},
             {"Selena: \"I've been meditating on your future. The visions are... unusual.\"", "Selena: \"I see you standing at a crossroads. One path leads home, the other leads deeper into this world.\"", "Selena: \"But here's the secret: both paths are the same. Home is not a place you leave, but a place you carry with you.\""},
             {"Selena: \"The moon whispers secrets tonight. It speaks of a dish that transcends worlds.\"", "Selena: \"Your yellow-braised chicken... it's not just a meal. It's a memory encoded with love and longing.\"", "Selena: \"And in this world, where memories become magic, that love is a power beyond measure.\""},
             {"Selena: \"I've been reading the stars. They align in a pattern I've never seen before.\"", "Selena: \"The constellation of the Fool overlaps with the Chariot, the Hermit, and... the World.\"", "Selena: \"You're not just collecting Personas. You're assembling a complete soul. A unified self.\""},
             {"Selena: \"The other night, I had a vision of you at a table. Not in this world, but not in the old one either.\"", "Selena: \"A place between places. Where the 22 Arcana gather not as cards, but as friends.\"", "Selena: \"And at the center of that table? A steaming pot of yellow-braised chicken. The ultimate bond, shared among kindred spirits.\""},
             {"Selena: \"My visions are growing stronger. I can see the threads connecting all things now.\"", "Selena: \"The monsters, the Personas, the Arcana... they're all expressions of the same force. Consciousness, longing, the will to exist.\"", "Selena: \"And your longing for that simple dish? It's the purest expression of all. Love, made tangible.\""},
             {"Selena: \"I see a final vision. You, standing before a great door.\"", "Selena: \"Behind it lies your home. But the door doesn't open with a key. It opens with a memory.\"", "Selena: \"The memory of sitting with friends, sharing a warm meal, feeling completely at peace. That is the true key. And you've already forged it, in every bond you've made here.\""},
         }},
        // Empress - Maria: tavern owner whose cooking reminds the hero of home
        {"sl_npc_3",
         {
             {"Maria: \"Welcome to my tavern! I don't recognize your face, but you look like you could use a warm meal.\"", "Maria: \"I make the best bread in town. But sometimes... I feel like something is missing from my recipes.\"", "Maria: \"You say my bread reminds you of something called 'yellow-braised chicken'? How intriguing!\""},
             {"Maria: \"I dreamt of a dish with golden sauce and tender meat. Is that what you describe?\"", "Maria: \"Cooking is my Persona, in a way. It brings comfort and strength to those who partake.\"", "Maria: \"The monsters drop strange ingredients. I've been experimenting with them in my kitchen.\""},
             {"Maria: \"You know, the Emperor himself visits my tavern. Even he can't resist my cooking.\"", "Maria: \"I've created a new recipe! It doesn't taste like your chicken, but it has heart.\"", "Maria: \"The way your eyes light up when you talk about that dish... it must have been wonderful.\""},
             {"Maria: \"I'm going to keep trying until I recreate that 'yellow-braised chicken' for you.\"", "Maria: \"Friend, even if I never get it quite right, know that every meal here is made with care.\"", "Maria: \"Because in this world, food is more than sustenance. It's a way of showing love.\""},
             {"Maria: \"I've been thinking about your stories. About the world you came from.\"", "Maria: \"You say everyone there eats this 'yellow-braised chicken'? It must be a dish of great cultural significance.\"", "Maria: \"In this world, food that carries such meaning... it becomes magical. Literally. The love in the recipe becomes power in the eater.\""},
             {"Maria: \"I tried to recreate your dish last night. I used monster meat, golden mushrooms, and a sauce made from star nectar.\"", "Maria: \"It wasn't the same, of course. But when I tasted it... I felt something. A warmth. A connection across worlds.\"", "Maria: \"I think I understand now. The dish isn't important. The feeling it carries is what matters.\""},
             {"Maria: \"I've been talking to the other Arcana holders. They all have their own comfort foods, their own memories of home.\"", "Maria: \"The Fool misses his chicken. The Magician misses his library. The Hermit... well, he misses his solitude.\"", "Maria: \"But we all gather here, at my tavern, and share our memories through food. That's the true magic of this place.\""},
             {"Maria: \"I've created something special. A 'World's Welcome Stew.' It combines ingredients from every Arcana's homeland.\"", "Maria: \"When you taste it, you'll feel the love of the Fool, the wisdom of the Magician, the warmth of the Empress... all together.\"", "Maria: \"Because home isn't one place. It's the sum of all the places we've been loved.\""},
             {"Maria: \"I think I've finally cracked it. The secret to your yellow-braised chicken.\"", "Maria: \"It's not the ingredients. It's not the technique. It's the intention behind it.\"", "Maria: \"When someone cooks for you with love, the dish carries a piece of their soul. And that soul-food nourishes more than the body.\""},
             {"Maria: \"Tonight, I'm making a feast. For you, for all of us who found each other in this strange world.\"", "Maria: \"And at the center of the table, I'll place a dish. It may not be your yellow-braised chicken...\"", "Maria: \"But it's made with the same love. The same longing. The same hope. And I hope it tastes like home.\""},
         }},
        // Emperor - Arthur: town guard captain
        {"sl_npc_4",
         {
             {"Arthur: \"State your business. These streets aren't safe at night, what with the monsters about.\"", "Arthur: \"The Emperor stands for authority and protection. I keep order in this town.\"", "Arthur: \"I've heard rumors of a traveler from another world. You, perhaps? No matter. All are welcome if they follow the law.\""},
             {"Arthur: \"The night monsters grow bolder. I need capable fighters to cull their numbers.\"", "Arthur: \"My guards report strange crystals appearing near monster nests. Handle with care; they may be dangerous.\"", "Arthur: \"You've proven yourself in battle. Would you consider assisting the guard on a more permanent basis?\""},
             {"Arthur: \"An Emperor is only as strong as the foundation he builds. This town is my foundation.\"", "Arthur: \"I respect strength, traveler. But I respect compassion even more. Do not lose yours.\"", "Arthur: \"The monsters seem drawn to something in this town. I suspect there's a greater threat lurking.\""},
             {"Arthur: \"With your help, the town is safer than it's been in months. The people owe you a debt.\"", "Arthur: \"But safety is fragile. One weak link, and the whole chain breaks.\"", "Arthur: \"You have the heart of a true warrior. Should you ever need an ally, the guard stands with you.\""},
             {"Arthur: \"I've been reviewing the guard's reports. The monster attacks are becoming more coordinated.\"", "Arthur: \"They're not random anymore. Something is directing them. Something intelligent.\"", "Arthur: \"I need you to investigate the northern ruins. That's where the largest concentration of monsters has been spotted.\""},
             {"Arthur: \"The northern ruins... I should warn you. They're not just old buildings.\"", "Arthur: \"They're a gateway. A threshold between this world and... somewhere else.\"", "Arthur: \"The monsters are coming from there. And if we don't stop them, they'll overrun the entire region.\""},
             {"Arthur: \"My scouts returned from the ruins. They didn't make it far, but what they saw was troubling.\"", "Arthur: \"Giant crystals growing from the ground. Glowing with the same light as those Star Shards the monsters drop.\"", "Arthur: \"And in the center of it all, a figure. Not a monster. Something... else. Something waiting.\""},
             {"Arthur: \"I've been thinking about leadership. About what it means to be an Emperor.\"", "Arthur: \"It's not about giving orders. It's about taking responsibility. For every life under your protection.\"", "Arthur: \"You understand that. I can see it in the way you fight. Not for glory, but for others. That's true strength.\""},
             {"Arthur: \"We're preparing for a final push against the ruins. I want you there, at the front.\"", "Arthur: \"Not because you're the strongest. But because you fight for the right reasons. And that inspires others.\"", "Arthur: \"When this is over, I'll buy you that 'yellow-braised chicken' you keep talking about. Whatever it takes.\""},
             {"Arthur: \"The battle is won. The ruins are sealed. And you... you played the most crucial part.\"", "Arthur: \"I don't know if you realize this, but you're not just a traveler anymore. You're one of us now.\"", "Arthur: \"This town is your home. These people are your family. And as long as I stand, no one will take that from you.\""},
         }},
        // Hierophant - Thomas: old church priest who knows the lore
        {"sl_npc_5",
         {
             {"Thomas: \"Welcome, child. The church is open to all who seek wisdom, even those from distant... very distant lands.\"", "Thomas: \"The Hierophant holds the keys to tradition and hidden knowledge. I have studied the 22 Arcana for decades.\"", "Thomas: \"You carry the scent of another world. Do not be alarmed; the divine sees all paths.\""},
             {"Thomas: \"The tarot cards are more than images. They are living archetypes that shape reality itself.\"", "Thomas: \"I sense a great hunger in you, child. Not just for food, but for belonging.\"", "Thomas: \"The monsters are drawn to this town because of the Arcana's power. You must be cautious.\""},
             {"Thomas: \"I have prepared holy water for the faithful. Take some; it may protect you in the dark.\"", "Thomas: \"The Fool's journey is one of self-discovery. Every Arcana you meet is a mirror to your soul.\"", "Thomas: \"I have seen the signs. A great change is coming, and you are at the center of it.\""},
             {"Thomas: \"Your bonds with the townspeople grow stronger. That is the true magic of this world.\"", "Thomas: \"Not the spells or the Personas, but the connections between hearts. That is divine power.\"", "Thomas: \"May the light guide you, traveler. And may you find the home you seek.\""},
             {"Thomas: \"I've been studying the ancient texts. The 22 Arcana weren't always separate.\"", "Thomas: \"Long ago, they were one. A single, complete being. The World, in its truest sense.\"", "Thomas: \"But it shattered. Broke into 22 pieces. And those pieces became the Arcana we know today.\""},
             {"Thomas: \"The question is: why did it shatter? And more importantly, can it be reassembled?\"", "Thomas: \"Some texts suggest that the pieces can be reunited. But only by someone who embodies all 22 aspects.\"", "Thomas: \"Someone who has known joy and sorrow, strength and weakness, beginnings and endings. Someone like... well. You know who.\""},
             {"Thomas: \"I've been praying on your behalf. The divine has shown me visions of your journey.\"", "Thomas: \"I see you carrying 10 Personas, switching between them as needed. The Fool's versatility. The Magician's adaptability.\"", "Thomas: \"But I also see you choosing one to embody fully. Not just carrying it, but becoming it. That is the path to true mastery.\""},
             {"Thomas: \"The other Arcana holders speak of you with respect. Even the Emperor, who trusts few.\"", "Thomas: \"You have a way of connecting with people. Of seeing their true selves beneath the masks they wear.\"", "Thomas: \"That is the gift of the Fool. The ability to see truth in chaos. To find order in the random.\""},
             {"Thomas: \"I've had a troubling vision. The shadows are gathering. Something ancient stirs in the deep.\"", "Thomas: \"The 22 Arcana were created to seal it away. And if they're not strong enough... it will return.\"", "Thomas: \"But the texts also speak of a hero. A Fool from another world, who came not by choice but by destiny. Who carries the memory of home like a shield.\""},
             {"Thomas: \"You are that hero. I've known it since we first met.\"", "Thomas: \"Not because you're the strongest. But because you have the most to lose. And the most to protect.\"", "Thomas: \"The Arcana are ready. The town is ready. And I believe... you are ready too. Go forth, child. Your destiny awaits.\""},
         }},
        // Chariot - Maxim: hot-blooded adventurer seeking strong foes
        {"sl_npc_6",
         {
             {"Maxim: \"Hey! You look strong! Let's fight! ...No? Then at least tell me where the strong monsters are!\"", "Maxim: \"The Chariot charges forward without fear! That's my motto, and it should be yours too!\"", "Maxim: \"I've heard rumors of a traveler who fights with multiple Personas. That must be you!\""},
             {"Maxim: \"There's nothing better than a good fight to make you feel alive! Come on, let's spar!\"", "Maxim: \"The monsters at night are getting stronger. I couldn't be more thrilled!\"", "Maxim: \"I collect trophies from my victories. These monster teeth are proof of my strength!\""},
             {"Maxim: \"You ever notice how the monsters drop weird stuff? I saw one drop a glowing crystal once!\"", "Maxim: \"Strength isn't just about muscles. It's about the will to keep going no matter what!\"", "Maxim: \"I want to be the strongest in this world! And that means defeating the strongest monsters!\""},
             {"Maxim: \"You're a worthy rival, friend! One day we'll have an all-out battle, just you and me!\"", "Maxim: \"But not today. Today, we fight side by side. And that's even better!\"", "Maxim: \"No matter how strong I get, I'll never forget the friends who fought beside me. Thanks, partner!\""},
             {"Maxim: \"I've been training with the Emperor's guard. They taught me formations, strategy... boring stuff.\"", "Maxim: \"But they also taught me something useful: how to channel my Persona's power into my strikes.\"", "Maxim: \"When you equip a Persona, don't just use its power. Become its power. That's the Chariot's way!\""},
             {"Maxim: \"I fought a monster last night that could copy my moves. Every punch, every dodge.\"", "Maxim: \"At first, I was frustrated. But then I realized: if it can copy me, it means my moves are worth copying!\"", "Maxim: \"That's the secret to the Chariot. Don't fear being imitated. Fear being ignored.\""},
             {"Maxim: \"I've been collecting Star Shards. Not for selling, but for something better.\"", "Maxim: \"I grind them up and mix them into my armor. Makes it glow. Makes me feel... more.\"", "Maxim: \"The Magician says it's dangerous. I say it's awesome. And you know what? Being awesome is half the battle!\""},
             {"Maxim: \"The Emperor told me something interesting. About the ruins in the north.\"", "Maxim: \"He said there's a monster there that's stronger than anything we've faced. A real challenge!\"", "Maxim: \"I can't wait to fight it! But... I think I'll need your help. Not because I can't win, but because it'll be more fun together!\""},
             {"Maxim: \"I've been thinking about strength. Real strength.\"", "Maxim: \"It's not about being the strongest. It's about protecting what matters. And you've taught me that.\"", "Maxim: \"When I fight now, I don't just fight for myself. I fight for the town, for my friends, for that weird chicken dish you keep talking about.\""},
             {"Maxim: \"I've never said this before, but... I'm glad you're here.\"", "Maxim: \"This world was getting boring before you showed up. Same monsters, same battles. But you? You bring something new.\"", "Maxim: \"A reason to fight that's bigger than just winning. A reason to be strong. And I wouldn't have it any other way.\""},
         }},
        // Strength - Reina: beast tamer who lives in harmony with monsters
        {"sl_npc_7",
         {
             {"Reina: \"Shh, you'll scare the creatures. They're not as dangerous as they seem, once you understand them.\"", "Reina: \"The Strength Arcana isn't about brute force. It's about compassion and inner courage.\"", "Reina: \"I sense a gentle soul in you, despite all the fighting. That is true strength.\""},
             {"Reina: \"The monsters are just trying to survive, like us. But I understand the need to defend the town.\"", "Reina: \"I've been gathering food for the little ones. Could you spare some bread?\"", "Reina: \"My animal friends found something strange in the forest. Would you help me investigate?\""},
             {"Reina: \"True power comes from understanding, not domination. Remember that in your darkest moments.\"", "Reina: \"The monsters respect me because I respect them. It's a balance we must all find.\"", "Reina: \"You've shown both courage and kindness. That is the essence of the Strength Arcana.\""},
             {"Reina: \"My friends and I are safe because of you. Thank you for protecting the balance.\"", "Reina: \"But remember: every life is precious. Even the monsters have a place in this world.\"", "Reina: \"No matter how strong you become, never lose the compassion that makes you human.\""},
             {"Reina: \"I've been studying the monsters' behavior. They're not attacking randomly.\"", "Reina: \"They're fleeing something. Something in the north. Something that scares even them.\"", "Reina: \"If we can understand what they're running from, maybe we can help them. And help ourselves.\""},
             {"Reina: \"Last night, a monster cub wandered into town. It was lost, scared, alone.\"", "Reina: \"I fed it, comforted it, and guided it back to the forest. And you know what? It didn't attack anyone.\"", "Reina: \"Violence begets violence. But kindness... kindness can break the cycle.\""},
             {"Reina: \"The other Arcana holders don't understand me. They think I'm weak for not wanting to fight.\"", "Reina: \"But I don't avoid conflict out of fear. I avoid it out of respect. For life, for balance, for the sanctity of existence.\"", "Reina: \"True Strength isn't about defeating enemies. It's about having the power to destroy, and choosing not to.\""},
             {"Reina: \"I've been working on something. A way to communicate with the monsters.\"", "Reina: \"Not words, exactly. More like... emotions. Intentions. The universal language of the heart.\"", "Reina: \"And I think I've figured out how to teach it to you. Interested?\""},
             {"Reina: \"The Emperor asked me to help with the northern campaign. I agreed, but on one condition.\"", "Reina: \"That we try to spare the monsters who surrender. That we don't kill indiscriminately.\"", "Reina: \"He agreed. Grudgingly, but he agreed. That's progress. That's the Strength Arcana at work.\""},
             {"Reina: \"You've shown me that the world can be a better place. Not through power, but through understanding.\"", "Reina: \"The monsters and humans don't have to be enemies. We can coexist. We can thrive together.\"", "Reina: \"And when the dust settles, when peace finally comes... I hope you'll still visit. The creatures miss you already.\""},
         }},
        // Hermit - Old Zhang: elderly hermit who knows secrets
        {"sl_npc_8",
         {
             {"Old Zhang: \"Heh, another young one lost in this world. Sit a while. The road is long.\"", "Old Zhang: \"The Hermit seeks truth in solitude. I've lived alone so long, the stones speak to me.\"", "Old Zhang: \"You smell like... no, it can't be. Not here. Not yellow-braised chicken? Impossible!\""},
             {"Old Zhang: \"I knew someone like you, long ago. Came from nowhere, spoke of impossible things.\"", "Old Zhang: \"The answer you seek isn't in battle or magic. It's in the memories you hold dear.\"", "Old Zhang: \"Bring me something from the old world. Coffee, perhaps? It reminds me of better days.\""},
             {"Old Zhang: \"I've seen 22 Arcana come and go. Each one is a lesson. Each one is a blessing.\"", "Old Zhang: \"The night monsters fear the light of memory. Hold your truth close, and they cannot harm you.\"", "Old Zhang: \"You're searching for a way home, aren't you? I've searched for fifty years. Maybe together...\""},
             {"Old Zhang: \"You remind me of myself when I was young. Full of hope, full of fire. Don't lose that.\"", "Old Zhang: \"If you ever find that yellow-braised chicken, save a bite for an old man, will you?\"", "Old Zhang: \"The journey of a thousand miles begins with a single step. And yours... well, it's just getting interesting.\""},
             {"Old Zhang: \"I've been watching the stars. They tell a story, if you know how to read them.\"", "Old Zhang: \"Your star... it's not from this sky. It fell here, just like you. Drawn by something. Or someone.\"", "Old Zhang: \"I think you were brought here for a reason. Not by accident. By design. By the very fabric of this world.\""},
             {"Old Zhang: \"I've studied the 22 Arcana my whole life. And I know their secret now.\"", "Old Zhang: \"They're not just power sources. They're keys. Keys to a door that was sealed long ago.\"", "Old Zhang: \"And the keyhole? It's shaped like a heart. A heart full of longing for something lost. Like your chicken.\""},
             {"Old Zhang: \"The ruins in the north... I've been there. Long ago, before the monsters came.\"", "Old Zhang: \"There was a temple. And in that temple, a door. And behind that door... well. I never found out.\"", "Old Zhang: \"But I felt something. A presence. Ancient, powerful, and somehow... familiar. Like a memory I couldn't place.\""},
             {"Old Zhang: \"I've been teaching myself to cook, you know. Tried to recreate that chicken of yours.\"", "Old Zhang: \"Failed miserably, of course. But the trying... the trying was fun. It reminded me that it's never too late to learn.\"", "Old Zhang: \"And learning is the only thing that never gets old. Not even when you're as old as me.\""},
             {"Old Zhang: \"I've had a vision. You, standing before that door in the northern temple.\"", "Old Zhang: \"And it's opening. Not because you found the key, but because you finally understood what the key was.\"", "Old Zhang: \"Your love for that dish. Your longing for home. That's the key. It always was.\""},
             {"Old Zhang: \"I won't live forever. But I've lived long enough to see what matters.\"", "Old Zhang: \"It's not power. It's not knowledge. It's the bonds we forge. The memories we share. The love we give.\"", "Old Zhang: \"And when you finally return home, tell them about me. Tell them that somewhere, in a strange world, an old man remembers you. And wishes you well.\""},
         }},
        // Wheel of Fortune - Fortuna: mysterious merchant
        {"sl_npc_9",
         {
             {"Fortuna: \"Welcome, welcome! Fortune favors the bold... and those with coin to spend!\"", "Fortuna: \"The Wheel turns for everyone, dearie. Today you may be up, tomorrow down. That's life!\"", "Fortuna: \"I trade in fate and fancy. Got any Star Shards? They're quite valuable, you know.\""},
             {"Fortuna: \"I saw your arrival in my crystal ball. The Fool, wandering between worlds. How exciting!\"", "Fortuna: \"Everything has a price. Information, power, even memories... especially memories of home.\"", "Fortuna: \"The monsters hoard shiny things. I've made a fortune buying their trinkets from adventurers.\""},
             {"Fortuna: \"Care to spin the wheel? No? Too bad. Your fortune would have been... interesting.\"", "Fortuna: \"I've met many travelers, but none who longed for chicken. You truly are unique.\"", "Fortuna: \"The Arcana are like cards in a game. Play them right, and you might just win your way home.\""},
             {"Fortuna: \"Your luck is changing, dearie. I can feel it in my bones. Great things await you!\"", "Fortuna: \"Remember: the wheel always turns. No matter how dark it gets, dawn will come.\"", "Fortuna: \"And when it does, I'll be here. Ready to trade. Ready to play. Ready to see what fortune brings.\""},
             {"Fortuna: \"I've been thinking about destiny. About whether anything is truly random.\"", "Fortuna: \"The Wheel of Fortune spins, yes. But what if it's not random? What if every spin is... guided?\"", "Fortuna: \"By memory. By longing. By the love of a simple dish that tastes like home. What if that's what spins the wheel?\""},
             {"Fortuna: \"I had a customer yesterday. Sold him a Star Shard. He used it to power his Persona.\"", "Fortuna: \"But before he left, he said something strange. He said the shard smelled like... chicken?\"", "Fortuna: \"I laughed. But now I'm wondering. What if the Star Shards are crystallized memories? And what if YOUR memories are the most powerful of all?\""},
             {"Fortuna: \"I've been tracking the market. Star Shard prices are skyrocketing. Everyone wants them.\"", "Fortuna: \"But you know what? I'm not selling mine. I'm saving them. For someone special.\"", "Fortuna: \"Someone who might need a little luck when they face the final challenge. Someone like... well. You know who.\""},
             {"Fortuna: \"The other Arcana holders come to me for advice. They want to know their fortune.\"", "Fortuna: \"I tell them what they want to hear. But with you? I tell you the truth. Because you can handle it.\"", "Fortuna: \"The truth is: the wheel is rigged. Always has been. But the house doesn't always win. Not when the player has a heart as pure as yours.\""},
             {"Fortuna: \"I've seen the final spin. The one that decides everything.\"", "Fortuna: \"It's not a game of chance. It's a game of choice. And the choice is yours.\"", "Fortuna: \"Stay and fight for this world, or return to your own. Both are valid. Both are brave. And neither is wrong.\""},
             {"Fortuna: \"Whatever you choose, dearie, know this: the wheel is grateful for your presence.\"", "Fortuna: \"You brought something to this world that it didn't have before. A reason to spin. A reason to hope.\"", "Fortuna: \"And if you ever need a friend, a trader, or just someone to watch the wheel with... you know where to find me.\""},
         }},
    };

    for (SocialLink *link : socialLinkManager_.allLinks())
    {
        if (!link)
            continue;
        auto it = kNpcDialogues.find(link->id());
        if (it == kNpcDialogues.end())
            continue;
        const auto &rankLines = it->second;
        for (int r = 0; r <= SocialLink::kMaxRank; ++r)
        {
            SocialLinkRankData data;
            if (r < static_cast<int>(rankLines.size()))
                data.dialogues = rankLines[r];
            else if (!rankLines.empty())
                data.dialogues = rankLines.back();
            else
                data.dialogues = {link->name() + ": \"...\""};
            // Every rank-up grants the current Persona one level.
            data.reward.personaLevels = 1;
            link->setRankData(r, std::move(data));
        }
    }
}
'''

# Find and replace the method
pattern = r'void GameManager::applyNpcDialogueTemplates\(\)[\s\S]*?(?=void GameManager::refreshDailyNpcs\(\))'
if re.search(pattern, content):
    content = re.sub(pattern, new_method, content)
    with open('src/manager/GameManager.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print('Successfully updated applyNpcDialogueTemplates')
else:
    print('Could not find the method to replace')
